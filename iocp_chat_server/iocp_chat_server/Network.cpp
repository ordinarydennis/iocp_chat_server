#include "Network.h"
#include "Define.h"
#include "NetworkConfig.h"
#include <iostream>
#include <string>

using namespace std::chrono;

Error Network::Init(const UINT16 port)
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
	
	error = BindandListen(port);
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
		printf("[ERROR] WSAStartup()�Լ� ���� : %d\n", WSAGetLastError());
		return Error::WSASTARTUP;
	}
	return Error::NONE;
}

void Network::SetMaxThreadCount()
{
	//WaingThread Queue�� ��� ���·� ���� ������� ���� ����Ǵ� ���� : (cpu���� * 2) + 1 
	mMaxThreadCount = static_cast<UINT16>(std::thread::hardware_concurrency() * 2 + 1);
}

Error Network::CreateListenSocket()
{
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == mListenSocket)
	{
		printf("[ERROR] socket()�Լ� ���� : %d\n", WSAGetLastError());
		return Error::SOCKET_CREATE_LISTEN;
	}
	return Error::NONE;
}

Error Network::CreateIOCP()
{
	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mMaxThreadCount);
	if (NULL == mIOCPHandle)
	{
		printf("[ERROR] CreateIoCompletionPort()�Լ� ����: %d\n", GetLastError());
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

Error Network::BindandListen(const UINT16 port)
{
	SOCKADDR_IN stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(port);		
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (0 != nRet)
	{
		printf("[ERROR] bind()�Լ� ���� : %d\n", WSAGetLastError());
		return Error::SOCKET_BIND;
	}

	nRet = listen(mListenSocket, 5);
	if (0 != nRet)
	{
		printf("[ERROR] listen()�Լ� ���� : %d\n", WSAGetLastError());
		return Error::SOCKET_LISTEN;
	}

	return Error::NONE;
}

Error Network::RegisterListenSocketToIOCP()
{
	HANDLE handle = CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, NULL, NULL);
	if (handle != mIOCPHandle)
	{
		printf("[ERROR] mListenSocket CreateIoCompletionPort ��� ����: %d\n", GetLastError());
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
	printf("WokerThread ����..\n");
}

void Network::WokerThread()
{
	//CompletionKey�� ���� ������ ����
	ClientInfo* pClientInfo = nullptr;
	//�Լ� ȣ�� ���� ����
	BOOL bSuccess = TRUE;
	//Overlapped I/O�۾����� ���۵� ������ ũ��
	DWORD dwIoSize = 0;
	//I/O �۾��� ���� ��û�� Overlapped ����ü�� ���� ������
	LPOVERLAPPED lpOverlapped = NULL;

	while (mIsWorkerRun)
	{
		bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
			&dwIoSize,					// ������ ���۵� ����Ʈ
			(PULONG_PTR)&pClientInfo,	// CompletionKey
			&lpOverlapped,				// Overlapped IO ��ü
			INFINITE);					// ����� �ð�
 
		if (NULL == lpOverlapped)
		{
			continue;
		}	

		//����� ������ ���� �޼��� ó��..
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
				printf("socket(%d) ���� ����\n", (int)pClientInfo->GetClientSocket());
				CloseSocket(pClientInfo);
				continue;
			}

			stOverlappedEx recvOverlappedEx = *pOverlappedEx;
			recvOverlappedEx.m_wsaBuf.len = dwIoSize;
			ProcRecvOperation(recvOverlappedEx);
			PostRecv(pClientInfo);
		}
		else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
		{
			if (dwIoSize == pOverlappedEx->m_wsaBuf.len)
			{
				pClientInfo->PopSendPacketPool();
			}
			else
			{
				printf("[ERROR] ���� %d �۽� ����.. ������ �õ�..\n", pClientInfo->GetId());
			}

			pClientInfo->SetSending(false);
		}
		else
		{
			printf("socket(%d)���� ���ܻ�Ȳ\n", (int)pClientInfo->GetClientSocket());
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
		bool isIdle = true;
		for (UINT32 index = 0; index < mClientInfos.size(); index++)
		{
			ClientInfo* clientInfo = &mClientInfos[index];
			if (clientInfo->IsSending())
			{ 
				//�̹� �������� ��Ŷ�� �ִ� ����
				continue;
			}

			auto sendPacketOpt = clientInfo->GetSendPacket();
			if (std::nullopt == sendPacketOpt)
			{
				//���� ��Ŷ�� ���� ����
				continue;
			}

			stPacket sendingPacket = sendPacketOpt.value();
			clientInfo->SetSending(true);
			SendData(sendingPacket);
			isIdle = false;
		}

		if (isIdle)
		{
			Sleep(1);
		}
	}
}

void Network::ProcAcceptOperation(const stOverlappedEx* pOverlappedEx)
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

void Network::ProcRecvOperation(const stOverlappedEx& recvOverlappedEx)
{
	AddToClientPoolRecvPacket(recvOverlappedEx);
}

//������ ������ ���� ��Ų��.
void Network::CloseSocket(ClientInfo* pClientInfo, const bool bIsForce)
{
	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER�� ����

	// bIsForce�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ� ���� ���� ��Ų��. ���� : ������ �ս��� ������ ���� 
	if (true == bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	//���� �ɼ��� �����Ѵ�.
	setsockopt(pClientInfo->GetClientSocket(), SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//���� ������ ���� ��Ų��. 
	pClientInfo->CloseSocket();
}

bool Network::PostSendMsg(ClientInfo* pClientInfo, const char* pMsg, const UINT32 len)
{
	DWORD dwRecvNumBytes = 0;

	//���۵� �޼����� ����
	CopyMemory(pClientInfo->GetSendBuf(), pMsg, len);
	pClientInfo->GetSendBuf()[len] = '\0';

	//Overlapped I/O�� ���� �� ������ ������ �ش�.
	pClientInfo->GetSendOverlappedEx()->m_wsaBuf.len = len;
	pClientInfo->GetSendOverlappedEx()->m_wsaBuf.buf = pClientInfo->GetSendBuf();
	pClientInfo->GetSendOverlappedEx()->m_eOperation = IOOperation::SEND;
	pClientInfo->GetSendOverlappedEx()->m_clientId = pClientInfo->GetId();

	int nRet = WSASend(pClientInfo->GetClientSocket(),
		&(pClientInfo->GetSendOverlappedEx()->m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED)(pClientInfo->GetSendOverlappedEx()),
		NULL);

	//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		printf("[����] WSASend()�Լ� ���� : %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool Network::PostRecv(ClientInfo* pClientInfo)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	//Overlapped I/O�� ���� �� ������ ������ �ش�.
	pClientInfo->GetRecvOverlappedEx()->m_wsaBuf.len = MAX_SOCKBUF;
	pClientInfo->GetRecvOverlappedEx()->m_wsaBuf.buf = pClientInfo->GetRecvBuf();
	pClientInfo->GetRecvOverlappedEx()->m_eOperation = IOOperation::RECV;
	pClientInfo->GetRecvOverlappedEx()->m_clientId = pClientInfo->GetId();

	int nRet = WSARecv(pClientInfo->GetClientSocket(),
		&(pClientInfo->GetRecvOverlappedEx()->m_wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED) (pClientInfo->GetRecvOverlappedEx()),
		NULL);

	//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		printf("[����] WSARecv()�Լ� ���� : %d\n", WSAGetLastError());
		return false;
	}

	return true;
}

void Network::SetAccepterThread()
{
	mAccepterThread = std::thread([this]() { AccepterThread(); });
	printf("AccepterThread ����..\n");
}

void Network::AccepterThread()
{
	while (mIsAccepterRun)
	{
		for (auto& clientInfo : mClientInfos)
		{
			clientInfo.AsyncAccept(mListenSocket);
		}

		Sleep(1);
	}
}

ClientInfo* Network::GetClientInfo(const UINT32 id)
{
	return id >= MAX_CLIENT ? nullptr : &mClientInfos.at(id);
}

//CompletionPort��ü�� ���ϰ� CompletionKey��
//�����Ű�� ������ �Ѵ�.
bool Network::BindIOCompletionPort(ClientInfo* pClientInfo)
{
	//socket�� pClientInfo�� CompletionPort��ü�� �����Ų��.
	auto hIOCP = CreateIoCompletionPort((HANDLE)pClientInfo->GetClientSocket()
		, mIOCPHandle
		, (ULONG_PTR)(pClientInfo), 0);

	if (NULL == hIOCP || mIOCPHandle != hIOCP)
	{
		printf("[����] CreateIoCompletionPort()�Լ� ����: %d\n", GetLastError());
		return false;
	}

	return true;
}

std::optional<stOverlappedEx> Network::GetClientRecvedPacket()
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	if (mClientPoolRecvedPacket.empty())
	{
		return std::nullopt;
	}
	
	stOverlappedEx front = mClientPoolRecvedPacket.front();
	mClientPoolRecvedPacket.pop();
	return front;
}

void Network::AddToClientPoolRecvPacket(const stOverlappedEx& recvOverlappedEx)
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	mClientPoolRecvedPacket.push(recvOverlappedEx);
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

void Network::SendData(const stPacket& packet)
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

//�����Ǿ��ִ� �����带 �ı��Ѵ�.
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