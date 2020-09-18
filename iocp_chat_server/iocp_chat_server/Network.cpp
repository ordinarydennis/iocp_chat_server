#pragma comment(lib, "ws2_32")

#include "Network.h"
#include "Define.h"
#include "NetworkConfig.h"

#include <iostream>
#include <string>


std::unique_ptr<Network> Network::mInstance = nullptr;
std::once_flag Network::mflag;

Network& Network::Instance()
{
	std::call_once(Network::mflag, []() {
		Network::mInstance.reset(new Network);
		});

	return *Network::mInstance;
}
void Network::Init()
{
	CreateSocket();
	BindandListen();
	CreateIOCP();
	CreateClient(MAX_CLIENT);
}
//소켓을 초기화하는 함수
void Network::CreateSocket()
{
	WSADATA wsaData;
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 != nRet)
	{
		printf("[ERROR] WSAStartup()함수 실패 : %d\n", WSAGetLastError());
		throw Error::WSASTARTUP;
	}

	//연결지향형 TCP , Overlapped I/O 소켓을 생성
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == mListenSocket)
	{
		printf("[에러] socket()함수 실패 : %d\n", WSAGetLastError());
		throw Error::SOCKET_CREATE;
	}
}
void Network::BindandListen()
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
		throw Error::SOCKET_BIND;
	}

	//접속 요청을 받아들이기 위해 cIOCompletionPort소켓을 등록하고 
	//접속대기큐를 5개로 설정 한다.
	nRet = listen(mListenSocket, 5);
	if (0 != nRet)
	{
		printf("[에러] listen()함수 실패 : %d\n", WSAGetLastError());
		throw Error::SOCKET_LISTEN;
	}
}
void Network::CreateIOCP()
{
	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (NULL == mIOCPHandle)
	{
		printf("[에러] CreateIoCompletionPort()함수 실패: %d\n", GetLastError());
		throw Error::IOCP_CREATE;
	}
}
void Network::CreateClient(const UINT32 maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; ++i)
	{
		mClientInfos.emplace_back();
	}
}
void Network::Run()
{
	SetWokerThread();

	SetAccepterThread();
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
	stClientInfo* pClientInfo = nullptr;
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
			(PULONG_PTR)&pClientInfo,		// CompletionKey
			&lpOverlapped,				// Overlapped IO 객체
			INFINITE);					// 대기할 시간

		//사용자 쓰레드 종료 메세지 처리..
		if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
		{
			mIsWorkerRun = false;
			continue;
		}

		if (NULL == lpOverlapped)
		{
			continue;
		}

		//client가 접속을 끊었을때..			
		if (FALSE == bSuccess || (0 == dwIoSize && TRUE == bSuccess))
		{
			printf("socket(%d) 접속 끊김\n", (int)pClientInfo->m_socketClient);

			stPacketHeader header;
			header.mSize = 0;
			char body[MAX_SOCKBUF] = { 0, };
			mReceivePacketPool.push(stPacket(pClientInfo->m_socketClient, header, body, MAX_SOCKBUF));

			CloseSocket(pClientInfo);
			continue;
		}

		//패킷 풀을 만든다.

		auto pOverlappedEx = (stOverlappedEx*)lpOverlapped;

		//Overlapped I/O Recv작업 결과 뒤 처리
		if (IOOperation::RECV == pOverlappedEx->m_eOperation)
		{
			pClientInfo->mRecvBuf[dwIoSize] = '\0';

			//헤더파싱
			stPacketHeader header;
			memcpy_s(&header.mSize, sizeof(UINT16), pClientInfo->mRecvBuf, sizeof(UINT16));
			memcpy_s(&header.mPacket_id, sizeof(UINT16), &pClientInfo->mRecvBuf[2], sizeof(UINT16));

			char body[MAX_SOCKBUF] = {0,};
			UINT32 bodySize = (UINT32)dwIoSize - PACKET_HEADER_SIZE;
			memcpy_s(body, bodySize, &pClientInfo->mRecvBuf[PACKET_HEADER_SIZE], bodySize);
		
			mReceivePacketPool.push(stPacket(pClientInfo->m_socketClient, header, body, bodySize));

			BindRecv(pClientInfo);
		}
		//Overlapped I/O Send작업 결과 뒤 처리
		else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
		{
			pClientInfo->mSendBuf[dwIoSize] = '\0';
			//헤더파싱
			stPacketHeader header;
			memcpy_s(&header.mSize, sizeof(UINT16), pClientInfo->mSendBuf, sizeof(UINT16));
			memcpy_s(&header.mPacket_id, sizeof(UINT16), &pClientInfo->mSendBuf[2], sizeof(UINT16));

			char body[MAX_SOCKBUF] = { 0, };
			UINT32 bodySize = (UINT32)dwIoSize - PACKET_HEADER_SIZE;
			memcpy_s(body, bodySize, &pClientInfo->mRecvBuf[PACKET_HEADER_SIZE], bodySize);

			printf("[송신] bytes : %d , msg : %s\n", dwIoSize, body);
		}
		//예외 상황
		else
		{
			printf("socket(%d)에서 예외상황\n", (int)pClientInfo->m_socketClient);
		}
	}
}
//소켓의 연결을 종료 시킨다.
void Network::CloseSocket(stClientInfo* pClientInfo, bool bIsForce)
{
	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER로 설정

// bIsForce가 true이면 SO_LINGER, timeout = 0으로 설정하여 강제 종료 시킨다. 주의 : 데이터 손실이 있을수 있음 
	if (true == bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	//socketClose소켓의 데이터 송수신을 모두 중단 시킨다.
	shutdown(pClientInfo->m_socketClient, SD_BOTH);

	//소켓 옵션을 설정한다.
	setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//소켓 연결을 종료 시킨다. 
	closesocket(pClientInfo->m_socketClient);

	pClientInfo->m_socketClient = INVALID_SOCKET;
}
//WSASend Overlapped I/O작업을 시킨다.
bool Network::SendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen)
{
	DWORD dwRecvNumBytes = 0;

	//전송될 메세지를 복사
	CopyMemory(pClientInfo->mSendBuf, pMsg, nLen);
	pClientInfo->mSendBuf[nLen] = '\0';

	//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
	pClientInfo->m_stSendOverlappedEx.m_wsaBuf.len = nLen;
	pClientInfo->m_stSendOverlappedEx.m_wsaBuf.buf = pClientInfo->mSendBuf;
	pClientInfo->m_stSendOverlappedEx.m_eOperation = IOOperation::SEND;

	int nRet = WSASend(pClientInfo->m_socketClient,
		&(pClientInfo->m_stSendOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED) & (pClientInfo->m_stSendOverlappedEx),
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
bool Network::BindRecv(stClientInfo* pClientInfo)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
	pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
	pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.buf = pClientInfo->mRecvBuf;
	pClientInfo->m_stRecvOverlappedEx.m_eOperation = IOOperation::RECV;

	int nRet = WSARecv(pClientInfo->m_socketClient,
		&(pClientInfo->m_stRecvOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED) & (pClientInfo->m_stRecvOverlappedEx),
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
		stClientInfo* pClientInfo = GetEmptyClientInfo();
		if (NULL == pClientInfo)
		{
			printf("[에러] Client Full\n");
			return;
		}

		//클라이언트 접속 요청이 들어올 때까지 기다린다.
		pClientInfo->m_socketClient = accept(mListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLen);
		if (INVALID_SOCKET == pClientInfo->m_socketClient)
		{
			continue;
		}

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
		printf("클라이언트 접속 : IP(%s) SOCKET(%d)\n", clientIP, (int)pClientInfo->m_socketClient);

		//클라이언트 갯수 증가
		++mClientCnt;
	}
}
stClientInfo* Network::GetEmptyClientInfo()
{
	for (auto& client : mClientInfos)
	{
		if (INVALID_SOCKET == client.m_socketClient)
		{
			return &client;
		}
	}

	return nullptr;
}
//CompletionPort객체와 소켓과 CompletionKey를
//연결시키는 역할을 한다.
bool Network::BindIOCompletionPort(stClientInfo* pClientInfo)
{
	//socket과 pClientInfo를 CompletionPort객체와 연결시킨다.
	auto hIOCP = CreateIoCompletionPort((HANDLE)pClientInfo->m_socketClient
		, mIOCPHandle
		, (ULONG_PTR)(pClientInfo), 0);

	if (NULL == hIOCP || mIOCPHandle != hIOCP)
	{
		printf("[에러] CreateIoCompletionPort()함수 실패: %d\n", GetLastError());
		return false;
	}

	return true;
}
stPacket Network::ReceivePacket()
{
	stPacket front = mReceivePacketPool.front();
	mReceivePacketPool.pop();
	return front;
}
bool Network::IsReceivePacketPoolEmpty()
{
	return mReceivePacketPool.empty();
}
void Network::AddPacket(const stPacket& p)
{
	mSendPacketPool.push(p);
}
bool Network::IsSendPacketPoolEmpty()
{
	return mSendPacketPool.empty();
}
stPacket Network::GetSendPacket()
{
	stPacket front = mSendPacketPool.front();
	mSendPacketPool.pop();
	return front;
}
void Network::SendData(UINT32 userId, char* pMsg, int nLen)
{
	std::vector<stClientInfo>::iterator ptr;
	for (ptr = mClientInfos.begin(); ptr != mClientInfos.end(); ++ptr)
	{
		if (ptr->m_socketClient != userId)
			continue;

		char buff[MAX_SOCKBUF] = { 0, };

		//헤더 정보 채워서 보낸다.
		stPacketHeader header;
		header.mSize = static_cast<UINT16>(strlen(pMsg) + PACKET_HEADER_SIZE);
		header.mPacket_id = 1;	
		memcpy_s(buff, sizeof(stPacketHeader), &header, sizeof(stPacketHeader));

		UINT32 bodySize = strlen(pMsg);
		memcpy_s(&buff[PACKET_HEADER_SIZE], bodySize, pMsg, bodySize);

		SendMsg(&(*ptr), buff, header.mSize);
		break;
	}
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