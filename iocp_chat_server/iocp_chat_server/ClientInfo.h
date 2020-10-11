#pragma once
#include "Define.h"
#include <mutex>
#include <optional>

class ClientInfo
{
public:
	ClientInfo() = default;

	ClientInfo(const ClientInfo& clientInfo);

	ClientInfo(const UINT32 id);
	
	UINT32			GetId() const { return mId;  };

	SOCKET			GetClientSocket() const { return mClientSocket; };

	stPacket		GetRecvPacket();

	std::optional<stPacket>	GetSendPacket();

	void			PopSendPacketPool();

	bool			IsSending();
	
	char*			GetRecvBuf() { return mRecvBuf; };

	char*			GetSendBuf() { return mSendBuf; };

	bool			IsConnecting();

	void			CloseSocket();

	void			PostAccept(SOCKET listenSocket);

	stOverlappedEx*	GetRecvOverlappedEx() { return &mRecvOverlappedEx; };

	stOverlappedEx*	GetSendOverlappedEx() { return &mSendOverlappedEx; };


	UINT64			GetLatestClosedTimeSec() const { return mLatestClosedTimeSec; }

	void			SetId(const UINT32 id) { mId = id; };

	void			SetClientSocket(const SOCKET clientSocket) { mClientSocket = clientSocket; };

	void			AddRecvPacket(const stPacket& packet);

	void			AddSendPacket(const stPacket& packet);

	void			SetRecvOverlappedEx(const stOverlappedEx& overlappedEx) { mRecvOverlappedEx = overlappedEx; };

	void			SetSendOverlappedEx(const stOverlappedEx& overlappedEx) { mSendOverlappedEx = overlappedEx; };

	void			SetSending(const bool bSending);

	void			SetIsConnecting(const bool isConnecting);
	
private:
	void			SetLatestClosedTimeSec(const UINT64 latestClosedTimeSec) { mLatestClosedTimeSec = latestClosedTimeSec; };

	bool			PostAccept(const SOCKET listenSocket, const UINT64 curTimeSec);

private:
	INT32						mId = 0;
	SOCKET						mClientSocket = INVALID_SOCKET;
	stOverlappedEx				mAcceptOverlappedEx;
	stOverlappedEx				mRecvOverlappedEx;
	stOverlappedEx				mSendOverlappedEx;

	std::queue<stPacket>        mRecvPacketPool;
	std::deque<stPacket>        mSendPacketPool;

	std::mutex                  mRecvPacketPoolLock;
	std::mutex                  mSendPacketPoolLock;

	char						mRecvBuf[MAX_SOCKBUF]	= { 0, };
	char						mSendBuf[MAX_SOCKBUF]	= { 0, };
	char						mAcceptBuf[64]			= { 0, };

	bool						m_bSending = false;
	std::mutex                  mSendingLock;

	bool						mIsConnecting = false;
	std::mutex                  mIsConnectingLock;

	UINT64						mLatestClosedTimeSec = 0;;
};