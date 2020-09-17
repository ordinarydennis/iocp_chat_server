#pragma once

#include <winsock2.h>
#include <Ws2tcpip.h>
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

struct stClientInfo
{
	SOCKET			m_socketClient;			
	stOverlappedEx	m_stRecvOverlappedEx;	
	stOverlappedEx	m_stSendOverlappedEx;	

	char			mRecvBuf[MAX_SOCKBUF]; 
	char			mSendBuf[MAX_SOCKBUF]; 

	stClientInfo()
	{
		ZeroMemory(&m_stRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&m_stSendOverlappedEx, sizeof(stOverlappedEx));
		m_socketClient = INVALID_SOCKET;
	}
};

struct stPacketHeader
{
	UINT16			mSize = 0;
	UINT16			mPacket_id = 0;
};

#define PACKET_HEADER_SIZE sizeof(stPacketHeader) + 1

struct stPacket
{
	UINT32			mClientId = 0;
	stPacketHeader	mHeader;
	char			mBody[MAX_SOCKBUF];

	stPacket(UINT32 ClientId, stPacketHeader Header, const char* Body, UINT16 size)
	{
		mClientId = ClientId;
		mHeader = Header;
		ZeroMemory(&mBody, MAX_SOCKBUF);
		memcpy_s(mBody, size, Body, size);
	}
};