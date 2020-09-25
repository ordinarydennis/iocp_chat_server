#pragma once
#include "Define.h"
#include <mutex>

class ClientInfo
{
public:
	ClientInfo() = default;
	ClientInfo(const ClientInfo&);
	ClientInfo(INT32 id);
	
	INT32			GetId() { return mId;  };
	SOCKET			GetClientSocket() { return mClientSocket; }
	stPacket		GetRecvPacket();
	stPacket		GetSendPacket();
	bool			IsSending() { return m_bSending; };
	
	char*			GetRecvBuf() { return mRecvBuf; };
	char*			GetSendBuf() { return mSendBuf; };

	stPacket		GetLastSendPacket() { return mLastSendPacket; };
	void			SetLastSendPacket(const stPacket& packet);

	stOverlappedEx*	GetRecvOverlappedEx() { return &m_stRecvOverlappedEx; };
	stOverlappedEx*	GetSendOverlappedEx() { return &m_stSendOverlappedEx; };

	void			SetId(UINT32 id) { mId = id; };
	void			SetClientSocket(SOCKET clientSocket);
	void			AddRecvPacket(stPacket p);
	void			AddSendPacket(stPacket p);
	void			SetRecvOverlappedEx(stOverlappedEx overlappedEx);
	void			SetSendOverlappedEx(const stOverlappedEx& overlappedEx);
	void			SetSending(bool bSending);

private:
	INT32						mId = 0;
	//TODO: stOverlappedEx에 있는걸로 대체하기
	SOCKET						mClientSocket = INVALID_SOCKET;
	stOverlappedEx				m_stRecvOverlappedEx;
	stOverlappedEx				m_stSendOverlappedEx;

	std::queue<stPacket>        mRecvPacketPool;
	std::queue<stPacket>        mSendPacketPool;

	std::mutex                  mRecvPacketPoolLock;
	std::mutex                  mSendPacketPoolLock;

	stPacket					mLastSendPacket;

	//TODO: stOverlappedEx에 있는걸로 대체하기
	char						mRecvBuf[MAX_SOCKBUF] = { 0, };
	char						mSendBuf[MAX_SOCKBUF] = { 0, };

	bool						m_bSending = false;
};