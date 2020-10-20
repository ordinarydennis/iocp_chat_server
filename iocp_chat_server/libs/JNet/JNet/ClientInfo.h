#pragma once

#include "Packet.h"
#include "Define.h"
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

		UINT32					GetId() const;

		void					SetId(const UINT32 id);
		
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

		void					SetRecvPacketBuff(const char* pData, const size_t dataSize);

		std::optional<JCommon::stPacket>	GetPacket();

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

		std::queue<JCommon::stPacket>        mRecvPacketPool;
		std::deque<JCommon::stPacket>        mSendPacketPool;

		std::mutex                  mRecvPacketPoolLock;
		std::mutex                  mSendPacketPoolLock;

		char						mRecvBuf[JCommon::MAX_SOCKBUF] = { 0, };
		char						mSendBuf[JCommon::MAX_SOCKBUF] = { 0, };
		char						mAcceptBuf[64] = { 0, };

		UINT64						mLatestClosedTimeSec = 0;

		LONG volatile				mIsSending = 0;

		//TODO 변수명 명확하게 수정
		char*						mRecvBuffer = nullptr;
		UINT32						mRecvPacketWPos = 0;
		UINT32						mRecvPacketRPos = 0;
	};
}

