#pragma once

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <queue>
#include "ServerConfig.h"
#include "NetworkConfig.h"

enum class Error
{
    NONE,
    WSASTARTUP = 1000,
    SOCKET_CREATE,
    SOCKET_BIND,
    SOCKET_LISTEN,
	IOCP_CREATE,
};

enum class PacketID : UINT16
{
	DEV_ECHO = 1,

	// 로그인
	LOGIN_REQ = 201,
	LOGIN_RES = 202,

	ROOM_ENTER_REQ = 206,
	ROOM_ENTER_RES = 207,
	ROOM_NEW_USER_NTF = 208,
	ROOM_USER_LIST_NTF = 209,

	ROOM_LEAVE_REQ = 215,
	ROOM_LEAVE_RES = 216,
	ROOM_LEAVE_USER_NTF = 217,

	ROOM_CHAT_REQ = 221,
	ROOM_CHAT_RES = 222,
	ROOM_CHAT_NOTIFY = 223,
};
enum class IOOperation
{
	RECV,
	SEND
};

//WSAOVERLAPPED구조체를 확장 시켜서 필요한 정보를 더 넣었다.
struct stOverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;		
	SOCKET		m_socketClient;			
	WSABUF		m_wsaBuf;				
	IOOperation m_eOperation;		
};

struct stPacketHeader
{
	UINT16			mSize = 0;
	UINT16			mPacket_id = 0;
};

#define PACKET_HEADER_SIZE sizeof(stPacketHeader) + 1

struct stPacket
{
	UINT32			mClientFrom = 0;
	UINT32			mClientTo = 0;
	stPacketHeader	mHeader;
	char			mBody[MAX_SOCKBUF];

	stPacket()
	{
		ZeroMemory(&mBody, MAX_SOCKBUF);
	};

	stPacket(UINT32 ClientFrom, UINT32 ClientTo, stPacketHeader Header, const char* Body, UINT16 size)
	{
		mClientFrom = ClientFrom;
		mClientTo = ClientTo;
		mHeader = Header;
		ZeroMemory(&mBody, MAX_SOCKBUF);
		memcpy_s(mBody, size, Body, size);
	}
};

struct stClientInfo
{
	INT32			mId = 0;
	//TODO: 변수명 통일
	SOCKET			m_socketClient;
	stOverlappedEx	m_stRecvOverlappedEx;
	stOverlappedEx	m_stSendOverlappedEx;

	std::queue<stPacket>        mRecvPacketPool;
	std::queue<stPacket>        mSendPacketPool;

	stPacket					mLastSendPacket;

	char			mRecvBuf[MAX_SOCKBUF] = {0,};
	char			mSendBuf[MAX_SOCKBUF] = {0,};

	bool			m_bSending = false;

	stClientInfo(INT32 id)
	{
		mId = id;
		ZeroMemory(&m_stRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&m_stSendOverlappedEx, sizeof(stOverlappedEx));
		m_socketClient = INVALID_SOCKET;
	}
};