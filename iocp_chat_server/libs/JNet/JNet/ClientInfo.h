#pragma once

#include "Packet.h"
#include "Define.h"
#include "SQueue.h"
#include <mutex>
#include <optional>
#include <queue>
#include <deque>

namespace JNet
{
	class ClientInfo
	{
	public:
		ClientInfo();

		ClientInfo(const ClientInfo& clientInfo);

		ClientInfo(const UINT32 id);

		~ClientInfo();

		UINT32					GetId() const;

		void					SetInfo(const UINT32 id, const size_t packetBuffSize = JCommon::MAX_SOCKBUF);
		
		stOverlappedEx*			GetRecvOverlappedEx();

		stOverlappedEx*			GetSendOverlappedEx();

		char*					GetRecvBuf();

		char*					GetSendBuf();

		SOCKET					GetClientSocket() const;

		void					SetClientSocket(const SOCKET clientSocket);

		void					CloseSocket();

		bool					IsSending();

		std::optional<JCommon::stPacket>	GetSendPacket();

		void					PopSendPacketPool();

		void					SetSending(const bool bSending);

		void					AddSendPacket(const JCommon::stPacket& packet);

		void					AsyncAccept(SOCKET listenSocket);

		std::optional<JCommon::stPacket>	RecvPacket(const char* pData, const size_t dataSize);

	private:
		void					Init();

		void					SetLatestClosedTimeSec(const UINT64 latestClosedTimeSec);

		bool					PostAccept(const SOCKET listenSocket);

		UINT64					GetLatestClosedTimeSec() const;

	private:
		INT32						mId = 0;

		SOCKET						mClientSocket = INVALID_SOCKET;

		stOverlappedEx				mAcceptOverlappedEx;
		stOverlappedEx				mRecvOverlappedEx;
		stOverlappedEx				mSendOverlappedEx;

		JNet::SQueue<JCommon::EntryPacket>		 mSendPacketQueue;

		//비동기 송수신에 사용되는 버퍼
		//char*						mRecvBuf = nullptr;
		//char*						mSendBuf = nullptr;

		char						mRecvBuf[JCommon::MAX_SOCKBUF] = { 0, };
		char						mSendBuf[JCommon::MAX_SOCKBUF] = { 0, };
		char						mAcceptBuf[64] = { 0, };

		UINT64						mLatestClosedTimeSec = 0;

		LONG volatile				mIsSending = 0;

		//수신된 패킷을 저장하는 버퍼
		char*						mRecvPacketPool = nullptr;
		UINT32						mRecvPacketWPos = 0;
		UINT32						mRecvPacketRPos = 0;
	};
}

