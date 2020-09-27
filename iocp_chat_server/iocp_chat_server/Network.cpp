#pragma comment(lib, "Ws2_32.lib")	//AcceptEx
#pragma comment(lib, "mswsock.lib") //AcceptEx

#include "Network.h"
#include "Define.h"
#include "NetworkConfig.h"
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <iostream>
#include <string>

void Network::Init(UINT16 SERVER_PORT)
{
	CreateListenSocket();
	CreateIOCP();
	CreateClient(MAX_CLIENT);
	BindandListen(SERVER_PORT);
	
}
//소켓을 초기화하는 함수
void Network::CreateListenSocket()
{
	WSADATA wsaData;
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 != nRet)
	{
		printf("[ERROR] WSAStartup()함수 실패 : %d\n", WSAGetLastError());
		return;
	}

	//연결지향형 TCP , Overlapped I/O 소켓을 생성
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == mListenSocket)
	{
		printf("[에러] socket()함수 실패 : %d\n", WSAGetLastError());
		return;
	}
}
void Network::BindandListen(UINT16 SERVER_PORT)
{
	SOCKADDR_IN		stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(SERVER_PORT); //서버 포트를 설정한다.		
	//어떤 주소에서 들어오는 접속이라도 받아들이겠다.
	//보통 서버라면 이렇게 설정한다. 만약 한 아이피에서만 접속을 받고 싶다면
	//그 주소를 inet_addr함수를 이용해 넣으면 된다.
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//위에서 지정한 서버 주소 정보와 cIOCompletionPort 소켓을 연결한다.
	int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (0 != nRet)
	{
		printf("[에러] bind()함수 실패 : %d\n", WSAGetLastError());
		return;
	}

	//접속 요청을 받아들이기 위해 cIOCompletionPort소켓을 등록하고 
	//접속대기큐를 5개로 설정 한다.
	nRet = listen(mListenSocket, 5);
	if (0 != nRet)
	{
		printf("[에러] listen()함수 실패 : %d\n", WSAGetLastError());
		return;
	}

	CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, NULL, NULL);
	if (SOCKET_ERROR == AsyncAccept(mListenSocket))
	{
		printf("[에러] AsyncAccept()함수 실패 : %d", WSAGetLastError());
		return;
	}
}
void Network::CreateIOCP()
{
	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (NULL == mIOCPHandle)
	{
		printf("[에러] CreateIoCompletionPort()함수 실패: %d\n", GetLastError());
		return;
	}
}
void Network::CreateClient(const UINT32 maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; i++)
	{
		mClientInfos.emplace_back(i);
		mClientInfos[i].SetId(i);
	}
}
void Network::Run()
{
	SetWokerThread();
	//SetAccepterThread();
}
void Network::SetWokerThread()
{
	//WaingThread Queue에 대기 상태로 넣을 쓰레드들 생성 권장되는 개수 : (cpu개수 * 2) + 1 
	for (int i = 0; i < MAX_WORKERTHREAD; i++)
	{
		mIOWorkerThreads.emplace_back([this]() { WokerThread(); });
	}
	printf("WokerThread 시작..\n");
}
void Network::WokerThread()
{
	//CompletionKey를 받을 포인터 변수
	ClientInfo* pClientInfo = nullptr;
	//함수 호출 성공 여부
	BOOL bSuccess = TRUE;
	//Overlapped I/O작업에서 전송된 데이터 크기
	DWORD dwIoSize = 0;
	//I/O 작업을 위해 요청한 Overlapped 구조체를 받을 포인터
	LPOVERLAPPED lpOverlapped = NULL;

	while (mIsWorkerRun)
	{
		bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
			&dwIoSize,					// 실제로 전송된 바이트
			(PULONG_PTR)&pClientInfo,	// CompletionKey
			&lpOverlapped,				// Overlapped IO 객체
			INFINITE);					// 대기할 시간
 
		if (NULL == lpOverlapped)
			continue;

		//사용자 쓰레드 종료 메세지 처리..
		if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
		{
			mIsWorkerRun = false;
			continue;
		}

		auto pOverlappedEx = (stOverlappedEx*)lpOverlapped;
		if (IOOperation::ACCEPT == pOverlappedEx->m_eOperation)
		{
			ProcAcceptOperation(pOverlappedEx);
			continue;
		}
		else if (IOOperation::RECV == pOverlappedEx->m_eOperation)
		{
			//check close client..			
			if (FALSE == bSuccess || (0 == dwIoSize && TRUE == bSuccess))
			{
				printf("socket(%d) 접속 끊김\n", (int)pClientInfo->GetClientSocket());
				CloseSocket(pClientInfo);
				continue;
			}

			ProcRecvOperation(pClientInfo, dwIoSize);
		}
		else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
		{
			//패킷 순서 확인
			//ProcSendOperation(pClientInfo, dwIoSize);
		}
		//예외
		else
		{
			printf("socket(%d)에서 예외상황\n", (int)pClientInfo->GetClientSocket());
		}
	}
}
void Network::ProcAcceptOperation(stOverlappedEx* pOverlappedEx)
{
	PSOCKADDR pRemoteSocketAddr = nullptr;
	PSOCKADDR pLocalSocketAddr = nullptr;
	INT pRemoteSocketAddrLength = 0;
	INT pLocalSocketAddrLength = 0;

	//정보 얻기
	GetAcceptExSockaddrs(
		pOverlappedEx->m_buffer, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		&pLocalSocketAddr, &pLocalSocketAddrLength, &pRemoteSocketAddr, &pRemoteSocketAddrLength);

	SOCKADDR_IN remoteAddr = *(reinterpret_cast<PSOCKADDR_IN>(pRemoteSocketAddr));
	//접속한 클라이언트 IP와 포트 정보 얻기
	char ip[24] = { 0, };
	inet_ntop(AF_INET, &remoteAddr.sin_addr, ip, sizeof(ip));
	printf("Accept New  IP %s PORT: %d \n", ip, ntohs(remoteAddr.sin_port));

	ClientInfo* pClientInfo = GetEmptyClientInfo();
	if (nullptr == pClientInfo)
	{
		return;
	}

	pClientInfo->SetClientSocket(pOverlappedEx->m_clientSocket);
	//I/O Completion Port객체와 소켓을 연결시킨다.
	bool bRet = BindIOCompletionPort(pClientInfo);
	if (false == bRet)
	{
		return;
	}

	//Recv Overlapped I/O작업을 요청해 놓는다.
	bRet = BindRecv(pClientInfo);
	if (false == bRet)
	{
		return;
	}

	//클라이언트 갯수 증가
	++mClientCnt;

	//accept 다시 등록
	AsyncAccept(mListenSocket);
}
void Network::ProcRecvOperation(ClientInfo* pClientInfo, DWORD dwIoSize)
{
	//헤더파싱
	stPacketHeader header;
	memcpy_s(&header.mSize, sizeof(UINT16), pClientInfo->GetRecvBuf(), sizeof(UINT16));
	memcpy_s(&header.mPacket_id, sizeof(UINT16), &pClientInfo->GetRecvBuf()[2], sizeof(UINT16));

	char body[MAX_SOCKBUF] = { 0, };
	UINT32 bodySize = (UINT32)dwIoSize - PACKET_HEADER_SIZE;
	memcpy_s(body, bodySize, &pClientInfo->GetRecvBuf()[PACKET_HEADER_SIZE], bodySize);

	pClientInfo->AddRecvPacket(stPacket(pClientInfo->GetId(), 0, header, body, bodySize));

	AddToClientPoolRecvPacket(pClientInfo);

	BindRecv(pClientInfo);
}
void Network::ProcSendOperation(ClientInfo* pClientInfo, DWORD dwIoSize)
{
	if (dwIoSize != pClientInfo->GetLastSendPacket().mHeader.mSize)
	{
		//전송이 안된 나머지 부분만 보내도록 수정
		printf("[ERROR] 송신 실패.. 재전송 시도..\n");
		pClientInfo->AddSendPacket(pClientInfo->GetLastSendPacket());
	}
	else
	{
		pClientInfo->SetSending(false);
		printf("유저 %d bytes : %d 송신 완료\n", pClientInfo->GetId(), dwIoSize);
	}
}
//소켓의 연결을 종료 시킨다.
void Network::CloseSocket(ClientInfo* pClientInfo, bool bIsForce)
{
	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER로 설정

// bIsForce가 true이면 SO_LINGER, timeout = 0으로 설정하여 강제 종료 시킨다. 주의 : 데이터 손실이 있을수 있음 
	if (true == bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	//socketClose소켓의 데이터 송수신을 모두 중단 시킨다.
	shutdown(pClientInfo->GetClientSocket(), SD_BOTH);

	//소켓 옵션을 설정한다.
	setsockopt(pClientInfo->GetClientSocket(), SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//소켓 연결을 종료 시킨다. 
	closesocket(pClientInfo->GetClientSocket());

	pClientInfo->SetClientSocket(INVALID_SOCKET);
}
//WSASend Overlapped I/O작업을 시킨다.
bool Network::SendMsg(ClientInfo* pClientInfo, char* pMsg, UINT32 len)
{
	DWORD dwRecvNumBytes = 0;

	//전송될 메세지를 복사
	CopyMemory(pClientInfo->GetSendBuf(), pMsg, len);
	pClientInfo->GetSendBuf()[len] = '\0';

	//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
	pClientInfo->GetSendOverlappedEx()->m_wsaBuf.len = len;
	pClientInfo->GetSendOverlappedEx()->m_wsaBuf.buf = pClientInfo->GetSendBuf();
	pClientInfo->GetSendOverlappedEx()->m_eOperation = IOOperation::SEND;

	int nRet = WSASend(pClientInfo->GetClientSocket(),
		&(pClientInfo->GetSendOverlappedEx()->m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED)(pClientInfo->GetSendOverlappedEx()),
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		printf("[에러] WSASend()함수 실패 : %d\n", WSAGetLastError());
		return false;
	}
	return true;
}
//WSARecv Overlapped I/O 작업을 시킨다.
bool Network::BindRecv(ClientInfo* pClientInfo)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
	pClientInfo->GetRecvOverlappedEx()->m_wsaBuf.len = MAX_SOCKBUF;
	pClientInfo->GetRecvOverlappedEx()->m_wsaBuf.buf = pClientInfo->GetRecvBuf();
	pClientInfo->GetRecvOverlappedEx()->m_eOperation = IOOperation::RECV;

	int nRet = WSARecv(pClientInfo->GetClientSocket(),
		&(pClientInfo->GetRecvOverlappedEx()->m_wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED) (pClientInfo->GetRecvOverlappedEx()),
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		printf("[에러] WSARecv()함수 실패 : %d\n", WSAGetLastError());
		return false;
	}

	return true;
}
void Network::SetAccepterThread()
{
	mAccepterThread = std::thread([this]() { AccepterThread(); });
	printf("AccepterThread 시작..\n");
}
void Network::AccepterThread()
{
	SOCKADDR_IN		stClientAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);

	while (mIsAccepterRun)
	{
		//접속을 받을 구조체의 인덱스를 얻어온다.
		ClientInfo* pClientInfo = GetEmptyClientInfo();
		if (NULL == pClientInfo)
		{
			printf("[에러] Client Full\n");
			return;
		}

		//클라이언트 접속 요청이 들어올 때까지 기다린다.
		SOCKET socketClient = accept(mListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLen);
		
		if (INVALID_SOCKET == socketClient)
		{
			continue;
		}

		pClientInfo->SetClientSocket(socketClient);

		//I/O Completion Port객체와 소켓을 연결시킨다.
		bool bRet = BindIOCompletionPort(pClientInfo);
		if (false == bRet)
		{
			return;
		}

		//Recv Overlapped I/O작업을 요청해 놓는다.
		bRet = BindRecv(pClientInfo);
		if (false == bRet)
		{
			return;
		}

		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
		printf("클라이언트 접속 : IP(%s) SOCKET(%d)\n", clientIP, (int)pClientInfo->GetClientSocket());

		//클라이언트 갯수 증가
		++mClientCnt;
	}
}
ClientInfo* Network::GetEmptyClientInfo()
{
	for (auto& client : mClientInfos)
	{
		if (INVALID_SOCKET == client.GetClientSocket())
		{
			return &client;
		}
	}

	return nullptr;
}
ClientInfo* Network::GetClientInfo(UINT32 id)
{
	return id >= MAX_CLIENT ? nullptr : &mClientInfos.at(id);
}
//CompletionPort객체와 소켓과 CompletionKey를
//연결시키는 역할을 한다.
bool Network::BindIOCompletionPort(ClientInfo* pClientInfo)
{
	//socket과 pClientInfo를 CompletionPort객체와 연결시킨다.
	auto hIOCP = CreateIoCompletionPort((HANDLE)pClientInfo->GetClientSocket()
		, mIOCPHandle
		, (ULONG_PTR)(pClientInfo), 0);

	if (NULL == hIOCP || mIOCPHandle != hIOCP)
	{
		printf("[에러] CreateIoCompletionPort()함수 실패: %d\n", GetLastError());
		return false;
	}

	return true;
}
ClientInfo* Network::GetClientRecvedPacket()
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	ClientInfo* front = mClientPoolRecvedPacket.front();
	mClientPoolRecvedPacket.pop();
	return front;
}
bool Network::IsEmptyClientPoolRecvPacket()
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	return mClientPoolRecvedPacket.empty();
}
void Network::AddToClientPoolRecvPacket(ClientInfo* c)
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	mClientPoolRecvedPacket.push(c);
}
BOOL Network::AsyncAccept(SOCKET listenSocket)
{
	BOOL ret = false;
	
	ZeroMemory(&mAcceptOverlappedEx->m_wsaOverlapped, sizeof(mAcceptOverlappedEx->m_wsaOverlapped));
	ZeroMemory(mAcceptOverlappedEx->m_buffer, MAX_SOCKBUF);
	mAcceptOverlappedEx->m_eOperation = IOOperation::ACCEPT;
	mAcceptOverlappedEx->m_clientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	DWORD bytes = 0;
	ret = AcceptEx(
		listenSocket, 
		mAcceptOverlappedEx->m_clientSocket,
		mAcceptOverlappedEx->m_buffer, 0,
		sizeof(SOCKADDR_IN) + 16, 
		sizeof(SOCKADDR_IN) + 16, 
		&bytes, 
		reinterpret_cast<LPOVERLAPPED>(&mAcceptOverlappedEx->m_wsaOverlapped)
	);

	return ret;
}
ClientInfo* Network::GetClientSendingPacket()
{
	std::lock_guard<std::mutex> guard(mSendPacketLock);
	ClientInfo* front = mClientPoolSendingPacket.front();
	mClientPoolSendingPacket.pop();
	return front;
}
bool Network::IsEmptyClientPoolSendPacket()
{
	std::lock_guard<std::mutex> guard(mSendPacketLock);
	return mClientPoolSendingPacket.empty();
}
void Network::AddToClientPoolSendPacket(ClientInfo* c)
{
	std::lock_guard<std::mutex> guard(mSendPacketLock);
	mClientPoolSendingPacket.push(c);
}
void Network::SendData(stPacket packet)
{
	//char* pMsg = packet.mBody;
	//int nLen = sizeof(packet.mBody);
	UINT16 packetId = packet.mHeader.mPacket_id;

	char buff[MAX_SOCKBUF] = { 0, };

	//헤더 정보 채워서 보낸다.
	stPacketHeader header;
	//header.mSize = static_cast<UINT16>(strlen(pMsg) + PACKET_HEADER_SIZE);
	header.mSize = packet.mHeader.mSize;
	// TODO: 여기 수정 해야함!
	header.mPacket_id = packetId;
	memcpy_s(buff, sizeof(stPacketHeader), &header, sizeof(stPacketHeader));

	//UINT32 bodySize = strlen(pMsg);
	UINT32 bodySize = packet.mHeader.mSize - PACKET_HEADER_SIZE;
	memcpy_s(&buff[PACKET_HEADER_SIZE], bodySize, packet.mBody, bodySize);

	ClientInfo* c = GetClientInfo(packet.mClientTo);
	SendMsg(c, buff, header.mSize);

}
//생성되어있는 쓰레드를 파괴한다.
void Network::DestroyThread()
{
	mIsWorkerRun = false;
	CloseHandle(mIOCPHandle);
	
	for (auto& th : mIOWorkerThreads)
	{
		if (th.joinable())
		{
			th.join();
		}
	}

	//Accepter 쓰레드를 종요한다.
	mIsAccepterRun = false;
	closesocket(mListenSocket);

	if (mAccepterThread.joinable())
	{
		mAccepterThread.join();
	}
}
void Network::Destroy()
{
	DestroyThread();
}