#include "Network.h"
#include "Logger.h"
#include <optional>

namespace JNet
{
	JCommon::ERROR_CODE Network::Init(const UINT32 maxClientCount, const UINT16 port, const size_t packetBuffSize)
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

		CreateClient(maxClientCount, packetBuffSize);

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
		//WaingThread Queue에 대기 상태로 넣을 쓰레드들 생성 권장되는 개수 : (cpu개수 * 2) + 1 
		mMaxThreadCount = static_cast<UINT16>(std::thread::hardware_concurrency() * 2 + 1);
	}

	JCommon::ERROR_CODE Network::WinsockStartup()
	{
		WSADATA wsaData;
		int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (0 != nRet)
		{
			JCommon::Logger::Error("WSAStartup()함수 실패 : %d", WSAGetLastError());
			return JCommon::ERROR_CODE::WSASTARTUP;
		}
		return JCommon::ERROR_CODE::NONE;
	}

	JCommon::ERROR_CODE Network::CreateListenSocket()
	{
		mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
		if (INVALID_SOCKET == mListenSocket)
		{
			JCommon::Logger::Error("socket()함수 실패 : %d", WSAGetLastError());
			return JCommon::ERROR_CODE::SOCKET_CREATE_LISTEN;
		}
		return JCommon::ERROR_CODE::NONE;
	}

	JCommon::ERROR_CODE Network::CreateIOCP()
	{
		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mMaxThreadCount);
		if (NULL == mIOCPHandle)
		{
			JCommon::Logger::Error("CreateIoCompletionPort()함수 실패: %d", GetLastError());
			return JCommon::ERROR_CODE::IOCP_CREATE;
		}
		return JCommon::ERROR_CODE::NONE;
	}

	void Network::CreateClient(const UINT32 maxClientCount, const size_t packetBuffSize)
	{
		for (UINT32 i = 0; i < maxClientCount; i++)
		{
			mClientInfos.emplace_back(packetBuffSize);
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
			JCommon::Logger::Error("bind()함수 실패 : %d", WSAGetLastError());
			return JCommon::ERROR_CODE::SOCKET_BIND;
		}

		nRet = listen(mListenSocket, 5);
		if (0 != nRet)
		{
			JCommon::Logger::Error("listen()함수 실패 : %d", WSAGetLastError());
			return JCommon::ERROR_CODE::SOCKET_LISTEN;
		}

		return JCommon::ERROR_CODE::NONE;
	}

	JCommon::ERROR_CODE Network::RegisterListenSocketToIOCP()
	{
		HANDLE handle = CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, NULL, NULL);
		if (handle != mIOCPHandle)
		{
			JCommon::Logger::Error("ListenSocket CreateIoCompletionPort 등록 실패: %d", GetLastError());
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
		JCommon::Logger::Info("WokerThread 시작..");
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
				bool isClosePacket = false;
				if (FALSE == bSuccess || (0 == dwIoSize && TRUE == bSuccess))
				{
					//header
					JCommon::stPacketHeader header;
					UINT32 clientId = pClientInfo->GetId();
					size_t bodySize = sizeof(clientId);
					header.mSize = static_cast<UINT16>(bodySize + JCommon::PACKET_HEADER_SIZE);
					header.mPacket_id = static_cast<UINT16>(JCommon::PACKET_ID::CLOSE_SOCKET);

					memcpy_s(&pClientInfo->GetRecvBuf()[0], JCommon::PACKET_HEADER_SIZE, &header, JCommon::PACKET_HEADER_SIZE);
					memcpy_s(&pClientInfo->GetRecvBuf()[JCommon::PACKET_HEADER_SIZE], bodySize, &clientId, bodySize);
					
					dwIoSize = header.mSize;

					isClosePacket = true;
				}

				ProcRecvOperation(pClientInfo, dwIoSize, isClosePacket);
			}
			else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
			{
				if (dwIoSize == pOverlappedEx->m_wsaBuf.len)
				{
					pClientInfo->PopSendPacketPool();
				}
				else
				{
					JCommon::Logger::Error("유저 % d 송신 실패..재전송 시도..", pClientInfo->GetId());
				}

				pClientInfo->SetSending(false);
			}
			else
			{
				JCommon::Logger::Error("socket(%d)에서 예외상황", (int)pClientInfo->GetClientSocket());
			}
		}
	}

	void Network::SetAccepterThread()
	{
		mAccepterThread = std::thread([this]() { AccepterThread(); });
		JCommon::Logger::Info("AccepterThread 시작..", GetLastError());
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

		//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
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

		//socket_error이면 client socket이 끊어진걸로 처리한다.
		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			JCommon::Logger::Error("WSARecv()함수 실패 : %d", WSAGetLastError());
			return false;
		}

		return true;
	}

	ClientInfo* Network::GetClientInfo(const UINT32 id)
	{
		return id >= mMaxClientCount ? nullptr : &mClientInfos.at(id);
	}

	void Network::ProcRecvOperation(ClientInfo* pClientInfo, const size_t size, bool isClosePacket)
	{
		auto completedPacket = pClientInfo->RecvPacket(pClientInfo->GetRecvBuf(), size);

		if (completedPacket.has_value())
		{
			PushRecvedPacket(completedPacket.value());
		}

		if (false == isClosePacket)
		{
			PostRecv(pClientInfo);
		}
	}

	void Network::PushRecvedPacket(const JCommon::stPacket& packet)
	{
		JCommon::EntryPacket pEntryPacket;
		pEntryPacket.mPacket = packet;
		mRecvedPacketQueue.Push(pEntryPacket);
	}

	std::optional<JCommon::stPacket> Network::GetRecvedPacket()
	{
		auto packet = mRecvedPacketQueue.Front();
		if (nullptr == packet)
		{
			return std::nullopt;
		}

		JCommon::stPacket recvedPacket = packet->mPacket;
		mRecvedPacketQueue.Pop();
		return recvedPacket;
	}

	void Network::CloseSocket(const UINT32 cliendId)
	{
		auto clientInfo = GetClientInfo(cliendId);
		if (nullptr == clientInfo)
		{
			JCommon::Logger::Error("Client id : %d closing socket is fail", cliendId);
			return;
		}
		CloseSocket(clientInfo);
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
			JCommon::Logger::Error("CreateIoCompletionPort()함수 실패: %d", GetLastError());
			return false;
		}

		return true;
	}

	void Network::CloseSocket(ClientInfo* pClientInfo, const bool bIsForce)
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
					//이미 전송중인 패킷이 있는 유저
					continue;
				}

				auto sendPacketOpt = clientInfo->GetSendPacket();
				if (std::nullopt == sendPacketOpt)
				{
					//보낼 패킷이 없는 유저
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

		//전송될 메세지를 복사
		CopyMemory(pClientInfo->GetSendBuf(), pMsg, len);
		pClientInfo->GetSendBuf()[len] = '\0';

		//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
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

		//socket_error이면 client socket이 끊어진걸로 처리한다.
		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			JCommon::Logger::Error("WSASend()함수 실패 : %d", WSAGetLastError());
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

