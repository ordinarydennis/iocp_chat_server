#include "Network.h"
#include "Define.h"
#include "NetworkConfig.h"
#include <iostream>
#include <string>

using namespace std::chrono;

Error Network::Init(UINT16 SERVER_PORT)
{
	SetMaxThreadCount();

	Error error = Error::NONE;
	error = WinsockStartup();
	if (Error::NONE != error)
		return error;
	
	error = CreateListenSocket();
	if (Error::NONE != error)
		return error;

	error = CreateIOCP();
	if (Error::NONE != error)
		return error;

	CreateClient(MAX_CLIENT);
	
	error = BindandListen(SERVER_PORT);
	if (Error::NONE != error)
		return error;

	error = RegisterListenSocketToIOCP();
	if (Error::NONE != error)
		return error;

	return Error::NONE;
}

Error Network::WinsockStartup()
{
	WSADATA wsaData;
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 != nRet)
	{
		printf("[ERROR] WSAStartup()함수 실패 : %d\n", WSAGetLastError());
		return Error::WSASTARTUP;
	}
	return Error::NONE;
}

void Network::SetMaxThreadCount()
{
	//WaingThread Queue에 대기 상태로 넣을 쓰레드들 생성 권장되는 개수 : (cpu개수 * 2) + 1 
	mMaxThreadCount = std::thread::hardware_concurrency() * 2 + 1;
}

Error Network::CreateListenSocket()
{
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == mListenSocket)
	{
		printf("[ERROR] socket()함수 실패 : %d\n", WSAGetLastError());
		return Error::SOCKET_CREATE_LISTEN;
	}
	return Error::NONE;
}

Error Network::CreateIOCP()
{
	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mMaxThreadCount);
	if (NULL == mIOCPHandle)
	{
		printf("[ERROR] CreateIoCompletionPort()함수 실패: %d\n", GetLastError());
		return Error::IOCP_CREATE;
	}
	return Error::NONE;
}

void Network::CreateClient(const UINT32 maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; i++)
	{
		mClientInfos.emplace_back(i);
		mClientInfos[i].SetId(i);
		mIdleClientIds.push(i);
	}
}

Error Network::BindandListen(UINT16 SERVER_PORT)
{
	SOCKADDR_IN stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(SERVER_PORT);		
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (0 != nRet)
	{
		printf("[ERROR] bind()함수 실패 : %d\n", WSAGetLastError());
		return Error::SOCKET_BIND;
	}

	nRet = listen(mListenSocket, 5);
	if (0 != nRet)
	{
		printf("[ERROR] listen()함수 실패 : %d\n", WSAGetLastError());
		return Error::SOCKET_LISTEN;
	}

	return Error::NONE;
}

Error Network::RegisterListenSocketToIOCP()
{
	HANDLE handle = CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, NULL, NULL);
	if (handle != mIOCPHandle)
	{
		printf("[ERROR] mListenSocket CreateIoCompletionPort 등록 실패: %d\n", GetLastError());
		return Error::SOCKET_REGISTER_IOCP;
	}

	return Error::NONE;
}

//Error Network::SetAsyncAccept()
//{
//	//TODO: 최흥배
//	// 서버를 실행을 하자말자 많은 접속이 발생했을 때나 클라이언트 접속과 해제가 빈번한 경우를 잘 처리하기 위해 Accept를 최대 연결 수만큼 미리 요청할 수 있습니다.
//	// 현재는 1개만 accept를 요청을 했는데 최대 연결 가능 수만큼 미리 accept 하도록 합니다.
//	//if (SOCKET_ERROR == AsyncAccept(mListenSocket))
//	//{
//	//	printf("[에러] AsyncAccept()함수 실패 : %d", WSAGetLastError());
//	//	return Error::SOCKET_ASYNC_ACCEPT;
//	//}
//	return Error::NONE;
//}

void Network::Run()
{
	SetWokerThread();
	SetAccepterThread();
	SetSendPacketThread();
}

void Network::SetWokerThread()
{
	for (int i = 0; i < mMaxThreadCount; i++)
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
			ProcSendOperation(pClientInfo, dwIoSize);
		}
		else
		{
			printf("socket(%d)에서 예외상황\n", (int)pClientInfo->GetClientSocket());
		}
	}
}

void Network::SetSendPacketThread()
{
	mSendPacketThread = std::thread([this]() { SendPacketThread(); });
}

void Network::SendPacketThread()
{
	while (mSendPacketRun)
	{
		if (IsEmptyClientPoolSendPacket())
		{
			Sleep(50);
			continue;
		}

		ClientInfo* clientInfo = GetClientSendingPacket();
		stPacket p = clientInfo->GetSendPacket();

		while (clientInfo->IsSending())
			Sleep(50);

		clientInfo->SetLastSendPacket(p);
		clientInfo->SetSending(true);
		SendData(p);
	}
}

void Network::ProcAcceptOperation(stOverlappedEx* pOverlappedEx)
{
	ClientInfo* pClientInfo = GetClientInfo(pOverlappedEx->m_clientId);
	if (nullptr == pClientInfo)
	{
		return;
	}

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

	++mClientCnt;
}

void Network::ProcRecvOperation(ClientInfo* pClientInfo, DWORD dwIoSize)
{
	AddToClientPoolRecvPacket(pClientInfo, dwIoSize);
	BindRecv(pClientInfo);
}

void Network::ProcSendOperation(ClientInfo* pClientInfo, DWORD dwIoSize)
{
	if (dwIoSize != pClientInfo->GetLastSendPacket().mHeader.mSize)
	{
		//제대로 전송이 안됐다면 풀에 가장 앞에 추가하여 다시 보내도록 한다.
		printf("[ERROR] 유저 %d 송신 실패.. %d 재전송 시도..\n", pClientInfo->GetId(), pClientInfo->GetLastSendPacket().mHeader.mPacket_id);
		pClientInfo->AddSendPacketAtFront(pClientInfo->GetLastSendPacket());
		AddToClientPoolSendPacket(pClientInfo);
	}
	else
	{
		printf("유저 %d bytes : id : %d, size: %d 송신 완료\n", pClientInfo->GetId(), pClientInfo->GetLastSendPacket().mHeader.mPacket_id, dwIoSize);
	}
	pClientInfo->SetLastSendPacket(stPacket());
	pClientInfo->SetSending(false);
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

	//TODO 최흥배
	// SO_LINGER로 바로 끊어버리므로 shutdown 안해도 괜찮습니다.
	//socketClose소켓의 데이터 송수신을 모두 중단 시킨다.
	//shutdown(pClientInfo->GetClientSocket(), SD_BOTH);

	//소켓 옵션을 설정한다.
	setsockopt(pClientInfo->GetClientSocket(), SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//소켓 연결을 종료 시킨다. 
	pClientInfo->CloseSocket();
	mIdleClientIds.push(pClientInfo->GetId());
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
	while (mIsAccepterRun)
	{
		for (auto& clientInfo : mClientInfos)
		{
			clientInfo.AsyncAccept(mListenSocket);
		}

		Sleep(50);
	}
}

//ClientInfo* Network::GetEmptyClientInfo()
//{
//	//TODO 최흥배
//	// 클라이언트 수가 많을 때는 사용하지 않는 객체를 찾는 것도 좀 부담 될 수 있으므로 
//	//사용하지 않는 객체의 인덱스 번호만 관리하고 있으면 쉽고 빠르게 찾을 수 있을 것 같습니다.
//	if (mIdleClientIds.empty())
//	{
//		return nullptr;
//	}
//	
//	UINT32 idleClientId = mIdleClientIds.front();
//	mIdleClientIds.pop();
//	return &mClientInfos[idleClientId];
//}

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

std::pair<ClientInfo*, size_t> Network::GetClientRecvedPacket()
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	std::pair<ClientInfo*, size_t> front = mClientPoolRecvedPacket.front();
	mClientPoolRecvedPacket.pop();
	return front;
}

bool Network::IsEmptyClientPoolRecvPacket()
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	return mClientPoolRecvedPacket.empty();
}

void Network::AddToClientPoolRecvPacket(ClientInfo* c, size_t size)
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	mClientPoolRecvedPacket.push(std::make_pair(c, size));
}

ClientInfo* Network::GetClientSendingPacket()
{
	std::lock_guard<std::mutex> guard(mSendPacketLock);
	ClientInfo* front = mClientPoolSendingPacket.front();
	mClientPoolSendingPacket.pop();
	return front;
}

void Network::SendPacket(const stPacket& packet)
{
	ClientInfo* clientInfo = GetClientInfo(packet.mClientTo);
	if (nullptr == clientInfo)
	{
		return;
	}

	clientInfo->AddSendPacket(packet);
	AddToClientPoolSendPacket(clientInfo);
}

bool Network::IsEmptyClientPoolSendPacket()
{
	std::lock_guard<std::mutex> guard(mSendPacketLock);
	return mClientPoolSendingPacket.empty();
}

void Network::AddToClientPoolSendPacket(ClientInfo* clientInfo)
{
	std::lock_guard<std::mutex> guard(mSendPacketLock);
	mClientPoolSendingPacket.push(clientInfo);
}

void Network::SendData(stPacket packet)
{
	UINT16 packetId = packet.mHeader.mPacket_id;
	char buff[MAX_SOCKBUF] = { 0, };

	stPacketHeader header;
	header.mSize = packet.mHeader.mSize;
	header.mPacket_id = packetId;
	memcpy_s(buff, sizeof(stPacketHeader), &header, sizeof(stPacketHeader));

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

	mSendPacketRun = false;
	if (mSendPacketThread.joinable())
	{
		mSendPacketThread.join();
	}
	  
	mIsAccepterRun = false;
	if (mAccepterThread.joinable())
	{
		mAccepterThread.join();
	}

	closesocket(mListenSocket);
}

void Network::Destroy()
{
	DestroyThread();
}