#pragma once
#include "Define.h"
#include <mutex>
#include <optional>

class ClientInfo
{
public:
	ClientInfo() = default;
	ClientInfo(const ClientInfo&);
	ClientInfo(UINT32 id);
	
	UINT32			GetId() const { return mId;  };
	SOCKET			GetClientSocket() { return mClientSocket; }
	stPacket		GetRecvPacket();
	std::optional<stPacket>	GetSendPacket();
	void			PopSendPacketPool();
	bool			IsSending();
	
	char*			GetRecvBuf() { return mRecvBuf; };
	char*			GetSendBuf() { return mSendBuf; };
	bool			IsConnecting();
	void			CloseSocket();
	void			AsyncAccept(SOCKET listenSocket);

	stOverlappedEx*	GetRecvOverlappedEx() { return &mRecvOverlappedEx; };
	stOverlappedEx*	GetSendOverlappedEx() { return &mSendOverlappedEx; };

	UINT64			GetLatestClosedTimeSec();

	void			SetId(UINT32 id) { mId = id; };
	void			SetClientSocket(SOCKET clientSocket);
	void			AddRecvPacket(const stPacket& packet);
	void			AddSendPacket(const stPacket& packet);
	void			AddSendPacketAtFront(const stPacket& packet);
	void			SetRecvOverlappedEx(stOverlappedEx overlappedEx);
	void			SetSendOverlappedEx(const stOverlappedEx& overlappedEx);
	void			SetSending(bool bSending);
	void			SetIsConnecting(bool isConnecting);
	
private:
	void			SetLatestClosedTimeSec(UINT64 latestClosedTimeSec);
	bool			PostAccept(SOCKET listenSock_, const UINT64 curTimeSec_);

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
	std::mutex                  mIsConnectingLick;

	UINT64						mLatestClosedTimeSec = 0;;
};