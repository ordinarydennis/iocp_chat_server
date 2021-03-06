#pragma once

#include "Common.h"
#include "Define.h"
#include "ClientInfo.h"
#include "SQueue.h"
#include <thread>
#include <vector>
#include <mutex>
#include <queue>
#include <functional>

namespace JNet
{
	class Network
	{
	public:
		Network() = default;

		~Network() = default;

		JCommon::ERROR_CODE Init(const UINT32 maxClientCount, const UINT16 port, const size_t packetBuffSize);

		void				Run();

		void				Destroy();

		ClientInfo*			GetClientInfo(const UINT32 id);

		void				SendData(const JCommon::stPacket& packet);

		std::function<void(JCommon::stPacket)> GetPacketSender();

		std::optional<JCommon::stPacket>		GetRecvedPacket();

		void					CloseSocket(const UINT32 cliendId);

	private:
		void					SetMaxThreadCount();

		JCommon::ERROR_CODE		WinsockStartup();

		JCommon::ERROR_CODE		CreateListenSocket();

		JCommon::ERROR_CODE		CreateIOCP();

		void					CreateClient(const UINT32 maxClientCount, const size_t packetBuffSize);

		JCommon::ERROR_CODE		BindandListen(const UINT16 port);

		JCommon::ERROR_CODE		RegisterListenSocketToIOCP();
		
		void				SetWokerThread();

		void				WokerThread();

		void				ProcAcceptOperation(const stOverlappedEx* pOverlappedEx);

		bool				BindIOCompletionPort(ClientInfo* pClientInfo);

		bool				PostRecv(ClientInfo* pClientInfo);

		void				ProcRecvOperation(ClientInfo* pClientInfo, const size_t size, bool isClosePacket);

		void				CloseSocket(ClientInfo* pClientInfo, const bool bIsForce = false);

		void				SetSendPacketThread();

		void				SendPacketThread();

		void				DestroyThread();

		bool				PostSendMsg(ClientInfo* pClientInfo, const char* pMsg, const UINT32 len);

		void				SetAccepterThread();

		void				AccepterThread();

		void				SendPacket(const JCommon::stPacket& packet);

		void				PushRecvedPacket(const JCommon::stPacket& packet);

	private:
		UINT32						mMaxClientCount = 0;
		UINT16                      mMaxThreadCount = 0;
		SOCKET                      mListenSocket = INVALID_SOCKET;
		HANDLE                  	mIOCPHandle = INVALID_HANDLE_VALUE;
		
		std::thread                 mAccepterThread;
		std::thread                 mSendPacketThread;

		std::vector<ClientInfo>     mClientInfos;
		std::vector<std::thread>    mIOWorkerThreads;

		bool	                   	mIsWorkerRun = true;
		bool	                   	mIsAccepterRun = true;
		bool                        mSendPacketRun = true;

		int			                mClientCnt = 0;

		JNet::SQueue<JCommon::EntryPacket>		mRecvedPacketQueue;
	};
}
