#include "Network.h"
#include <optional>

namespace JNet
{
	JCommon::ERROR_CODE Network::Init(const UINT32 maxClientCount, const UINT16 port)
	{
		SetMaxThreadCount();

		JCommon::ERROR_CODE errorCode = JCommon::ERROR_CODE::NONE;
		
		errorCode = WinsockStartup();
		if (JCommon::ERROR_CODE::NONE != errorCode)
		{
			return errorCode;
		}

		errorCode = CreateListenSocket();
		if (JCommon::ERROR_CODE::NONE != errorCode)
		{
			return errorCode;
		}

		errorCode = CreateIOCP();
		if (JCommon::ERROR_CODE::NONE != errorCode)
		{
			return errorCode;
		}

		CreateClient(maxClientCount);

		errorCode = BindandListen(port);
		if (JCommon::ERROR_CODE::NONE != errorCode)
		{
			return errorCode;
		}

		errorCode = RegisterListenSocketToIOCP();
		if (JCommon::ERROR_CODE::NONE != errorCode)
		{
			return errorCode;
		}

		return JCommon::ERROR_CODE::NONE;
	}

	void Network::SetMaxThreadCount()
	{
		//WaingThread Queue�� ��� ���·� ���� ������� ���� ����Ǵ� ���� : (cpu���� * 2) + 1 
		mMaxThreadCount = static_cast<UINT16>(std::thread::hardware_concurrency() * 2 + 1);
	}

	JCommon::ERROR_CODE Network::WinsockStartup()
	{
		WSADATA wsaData;
		int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (0 != nRet)
		{
			printf("[ERROR] WSAStartup()�Լ� ���� : %d\n", WSAGetLastError());
			return JCommon::ERROR_CODE::WSASTARTUP;
		}
		return JCommon::ERROR_CODE::NONE;
	}

	JCommon::ERROR_CODE Network::CreateListenSocket()
	{
		mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
		if (INVALID_SOCKET == mListenSocket)
		{
			printf("[ERROR] socket()�Լ� ���� : %d\n", WSAGetLastError());
			return JCommon::ERROR_CODE::SOCKET_CREATE_LISTEN;
		}
		return JCommon::ERROR_CODE::NONE;
	}

	JCommon::ERROR_CODE Network::CreateIOCP()
	{
		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mMaxThreadCount);
		if (NULL == mIOCPHandle)
		{
			printf("[ERROR] CreateIoCompletionPort()�Լ� ����: %d\n", GetLastError());
			return JCommon::ERROR_CODE::IOCP_CREATE;
		}
		return JCommon::ERROR_CODE::NONE;
	}

	void Network::CreateClient(const UINT32 maxClientCount)
	{
		for (UINT32 i = 0; i < maxClientCount; i++)
		{
			mClientInfos.emplace_back(i);
			mClientInfos[i].SetId(i);
		}

		mMaxClientCount = maxClientCount;
	}

	JCommon::ERROR_CODE Network::BindandListen(const UINT16 port)
	{
		SOCKADDR_IN stServerAddr;
		stServerAddr.sin_family = AF_INET;
		stServerAddr.sin_port = htons(port);
		stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
		if (0 != nRet)
		{
			printf("[ERROR] bind()�Լ� ���� : %d\n", WSAGetLastError());
			return JCommon::ERROR_CODE::SOCKET_BIND;
		}

		nRet = listen(mListenSocket, 5);
		if (0 != nRet)
		{
			printf("[ERROR] listen()�Լ� ���� : %d\n", WSAGetLastError());
			return JCommon::ERROR_CODE::SOCKET_LISTEN;
		}

		return JCommon::ERROR_CODE::NONE;
	}

	JCommon::ERROR_CODE Network::RegisterListenSocketToIOCP()
	{
		HANDLE handle = CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, NULL, NULL);
		if (handle != mIOCPHandle)
		{
			printf("[ERROR] mListenSocket CreateIoCompletionPort ��� ����: %d\n", GetLastError());
			return JCommon::ERROR_CODE::SOCKET_REGISTER_IOCP;
		}

		return JCommon::ERROR_CODE::NONE;
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

	bool Network::PostRecv(ClientInfo* pClientInfo)
	{
		DWORD dwFlag = 0;
		DWORD dwRecvNumBytes = 0;

		//Overlapped I/O�� ���� �� ������ ������ �ش�.
		pClientInfo->GetRecvOverlappedEx()->m_wsaBuf.len = JCommon::MAX_SOCKBUF;
		pClientInfo->GetRecvOverlappedEx()->m_wsaBuf.buf = pClientInfo->GetRecvBuf();
		pClientInfo->GetRecvOverlappedEx()->m_eOperation = IOOperation::RECV;
		pClientInfo->GetRecvOverlappedEx()->m_clientId = pClientInfo->GetId();

		int nRet = WSARecv(pClientInfo->GetClientSocket(),
			&(pClientInfo->GetRecvOverlappedEx()->m_wsaBuf),
			1,
			&dwRecvNumBytes,
			&dwFlag,
			(LPWSAOVERLAPPED)(pClientInfo->GetRecvOverlappedEx()),
			NULL);

		//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			printf("[����] WSARecv()�Լ� ���� : %d\n", WSAGetLastError());
			return false;
		}

		return true;
	}

	ClientInfo* Network::GetClientInfo(const UINT32 id)
	{
		return id >= mMaxClientCount ? nullptr : &mClientInfos.at(id);
	}

	void Network::ProcRecvOperation(const stOverlappedEx& recvOverlappedEx)
	{
		AddToClientPoolRecvPacket(recvOverlappedEx);
	}

	void Network::AddToClientPoolRecvPacket(const stOverlappedEx& recvOverlappedEx)
	{
		std::lock_guard<std::mutex> guard(mRecvPacketLock);
		mClientPoolRecvedPacket.push(recvOverlappedEx);
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

				JCommon::stPacket sendingPacket = sendPacketOpt.value();
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

	void Network::SendData(const JCommon::stPacket& packet)
	{
		UINT16 packetId = packet.mHeader.mPacket_id;
		char buff[JCommon::MAX_SOCKBUF] = { 0, };

		JCommon::stPacketHeader header;
		header.mSize = packet.mHeader.mSize;
		header.mPacket_id = packetId;
		memcpy_s(buff, sizeof(JCommon::stPacketHeader), &header, sizeof(JCommon::stPacketHeader));

		UINT32 bodySize = packet.mHeader.mSize - JCommon::PACKET_HEADER_SIZE;
		memcpy_s(&buff[JCommon::PACKET_HEADER_SIZE], bodySize, packet.mBody, bodySize);

		ClientInfo* c = GetClientInfo(packet.mClientTo);
		PostSendMsg(c, buff, header.mSize);
	}

	std::function<void(JCommon::stPacket)> Network::GetPacketSender()
	{
		return std::bind(&Network::SendPacket, this, std::placeholders::_1);
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

	void Network::SendPacket(const JCommon::stPacket& packet)
	{
		ClientInfo* clientInfo = GetClientInfo(packet.mClientTo);
		if (nullptr == clientInfo)
		{
			return;
		}

		clientInfo->AddSendPacket(packet);
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

	void Network::Destroy()
	{
		DestroyThread();
		WSACleanup();
	}

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

}
