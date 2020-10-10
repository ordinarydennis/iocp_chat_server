#include "Network.h"
#include "Define.h"
#include "NetworkConfig.h"
#include <iostream>
#include <string>

using namespace std::chrono;

Error Network::Init(UINT16 serverPort)
{
	SetMaxThreadCount();

	Error error = Error::NONE;
	error = WinsockStartup();
	if (Error::NONE != error)
	{
		return error;
	}
	
	error = CreateListenSocket();
	if (Error::NONE != error)
	{
		return error;
	}
		
	error = CreateIOCP();
	if (Error::NONE != error)
	{
		return error;
	}

	CreateClient(MAX_CLIENT);
	
	error = BindandListen(serverPort);
	if (Error::NONE != error)
	{
		return error;
	}

	error = RegisterListenSocketToIOCP();
	if (Error::NONE != error)
	{
		return error;
	}

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
	}
}

//TODO 최흥배
// 변수 이름 일관성을 지켜주세요~
Error Network::BindandListen(UINT16 serverPort)
{
	SOCKADDR_IN stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(serverPort);		
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
		{
			continue;
		}	

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
			if (dwIoSize == pOverlappedEx->m_wsaBuf.len)
			{
				pClientInfo->PopSendPacketPool();
			}
			else
			{
				printf("[ERROR] 유저 %d 송신 실패.. 재전송 시도..\n", pClientInfo->GetId());
			}

			pClientInfo->SetSending(false);
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
		for (UINT32 index = 0; index < mClientInfos.size(); index++)
		{
			ClientInfo* clientInfo = &mClientInfos[index];
			if (clientInfo->IsSending())
			{ 
				//이미 전송중인 패킷이 있는 유저
				continue;
			}

			auto sendPacketOpt = clientInfo->GetSendPacket();
			if (std::nullopt == sendPacketOpt)
			{
				//보낼 패킷이 없는 유저
				continue;
			}

			stPacket sendingPacket = sendPacketOpt.value();
			clientInfo->SetSending(true);
			SendData(sendingPacket);
		}
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

	bRet = PostRecv(pClientInfo);
	if (false == bRet)
	{
		return;
	}

	++mClientCnt;
}

void Network::ProcRecvOperation(ClientInfo* pClientInfo, DWORD dwIoSize)
{
	AddToClientPoolRecvPacket(pClientInfo, dwIoSize);
	PostRecv(pClientInfo);
}

//void Network::ProcSendOperation(ClientInfo* pClientInfo, DWORD dwIoSize)
//{
//	//TODO 최흥배
//	// 데이터를 다 보내었는지 조사하는데 send 패킷 풀에서 데이터를 뺄 필요가 없습니다.
//	// 이 함수를 호출하는 stOverlappedEx에 보면 이미 보낼 데이터를 가리키는 포인터와 보내는 양이 기록 되어 있습니다.	
//	stPacket lastSendingPacket = pClientInfo->GetSendPacket().value();
//	if (dwIoSize == lastSendingPacket.mHeader.mSize)
//	{
//		//전송이 완료 됐을때만 pop
//		pClientInfo->PopSendPacketPool();
//	}
//	else
//	{
//		printf("[ERROR] 유저 %d 송신 실패.. 재전송 시도..\n", pClientInfo->GetId());
//	}
//
//	pClientInfo->SetSending(false);
//}

//소켓의 연결을 종료 시킨다.
void Network::CloseSocket(ClientInfo* pClientInfo, bool bIsForce)
{
	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER로 설정

	// bIsForce가 true이면 SO_LINGER, timeout = 0으로 설정하여 강제 종료 시킨다. 주의 : 데이터 손실이 있을수 있음 
	if (true == bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	//소켓 옵션을 설정한다.
	setsockopt(pClientInfo->GetClientSocket(), SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//소켓 연결을 종료 시킨다. 
	pClientInfo->CloseSocket();
}

//TODO 최흥배
// 비동기 요청을 호출하는 함수 이름에는 대부분 Post를 붙였으니 여기도 함수 이름에 Post를 붙이는 것이 일관성이 있지 않을까요?
//WSASend Overlapped I/O작업을 시킨다.
bool Network::PostSendMsg(ClientInfo* pClientInfo, char* pMsg, UINT32 len)
{
	DWORD dwRecvNumBytes = 0;

	//전송될 메세지를 복사
	CopyMemory(pClientInfo->GetSendBuf(), pMsg, len);
	pClientInfo->GetSendBuf()[len] = '\0';

	//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
	pClientInfo->GetSendOverlappedEx()->m_wsaBuf.len = len;
	pClientInfo->GetSendOverlappedEx()->m_wsaBuf.buf = pClientInfo->GetSendBuf();
	pClientInfo->GetSendOverlappedEx()->m_eOperation = IOOperation::SEND;

	//TODO 이부분 다시 조사하기
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

//TODO 최흥배
// 비동기 요청에는 보통 Post나 혹은 Begin 또는 Async를 붙입니다. Bind는 그 뜻이 너무 다른 것 같습니다.
//WSARecv Overlapped I/O 작업을 시킨다.
bool Network::PostRecv(ClientInfo* pClientInfo)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	//TODO 최흥배
	// 아래 버그입니다.
	// 받기를 완료하면 버퍼의 주소를 패킷처리 스레드로 넘기는데 만약 패킷 처리 스레드가 이 데이터를 처리하기 전에 해당 클라이언트가 패킷을 보내면 앞에 보낸 데이터를 덮어쓰게 됩니다.

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
			clientInfo.PostAccept(mListenSocket);
		}

		Sleep(1);
	}
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

std::optional<std::pair<ClientInfo*, size_t>> Network::GetClientRecvedPacket()
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	if (mClientPoolRecvedPacket.empty())
	{
		return std::nullopt;
	}
	
	std::pair<ClientInfo*, size_t> front = mClientPoolRecvedPacket.front();
	mClientPoolRecvedPacket.pop();
	return front;
}

void Network::AddToClientPoolRecvPacket(ClientInfo* c, size_t size)
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	mClientPoolRecvedPacket.push(std::make_pair(c, size));
}

void Network::SendPacket(const stPacket& packet)
{
	ClientInfo* clientInfo = GetClientInfo(packet.mClientTo);
	if (nullptr == clientInfo)
	{
		return;
	}

	clientInfo->AddSendPacket(packet);
}

std::function<void(stPacket)> Network::GetPacketSender()
{
	return std::bind(&Network::SendPacket, this, std::placeholders::_1);
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
	PostSendMsg(c, buff, header.mSize);
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