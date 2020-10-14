#pragma once

#include "NetworkConfig.h"
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <queue>

enum class Error
{
    NONE,
	IP = 1000,
	PORT,
    WSASTARTUP,
	SOCKET_CREATE_LISTEN,
    SOCKET_CREATE,
	IOCP_CREATE,
	SOCKET_BIND,
	SOCKET_LISTEN,
	SOCKET_REGISTER_IOCP,
	SOCKET_ASYNC_ACCEPT,
	REDIS_CONNECT,
};

enum class ERROR_CODE : UINT16
{
	NONE = 0,
	LOGIN_USER_INVALID_PW = 33,
};

enum class PacketID : UINT16
{
	DEV_ECHO = 1,

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
	NONE,
	ACCEPT,
	RECV,
	SEND
};

//WSAOVERLAPPED����ü�� Ȯ�� ���Ѽ� �ʿ��� ������ �� �־���.
struct stOverlappedEx
{
	WSAOVERLAPPED	m_wsaOverlapped;			//����ü�� ���� ù��°�� �;� �Ѵ�. ����� �����͸� ���� �� �ִ�.	
	WSABUF			m_wsaBuf;				
	IOOperation		m_eOperation;	
	UINT32			m_clientId;					
	stOverlappedEx()
	{
		ZeroMemory(&m_wsaOverlapped, sizeof(m_wsaOverlapped));
		m_eOperation	= IOOperation::NONE;
		m_wsaBuf.buf	= nullptr;
		m_wsaBuf.len	= 0;
		m_clientId		= 0;
	}
};

struct stPacketHeader
{
	UINT16			mSize = 0;
	UINT16			mPacket_id = 0;
};

const UINT16 PACKET_HEADER_SIZE = sizeof(stPacketHeader) + 1;

struct stPacket
{
	UINT32			mClientFrom = 0;
	UINT32			mClientTo = 0;
	stPacketHeader	mHeader;
	char			mBody[MAX_SOCKBUF] = { 0, };

	stPacket()
	{
		ZeroMemory(&mBody, MAX_SOCKBUF);
	};

	stPacket(UINT32 ClientFrom, UINT32 ClientTo, stPacketHeader Header, const char* Body, size_t size)
	{
		mClientFrom = ClientFrom;
		mClientTo = ClientTo;
		mHeader = Header;
		memcpy_s(mBody, size, Body, size);
	}

	size_t GetBodySize() const
	{
		return mHeader.mSize - PACKET_HEADER_SIZE;
	}
};

UINT64 GetCurTimeSec();