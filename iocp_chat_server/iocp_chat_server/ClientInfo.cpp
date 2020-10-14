#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#include "ClientInfo.h"
#include <mswsock.h>

using namespace std::chrono;

ClientInfo::ClientInfo(const ClientInfo& clientInfo)
{
	mId = clientInfo.mId;
	ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
	ZeroMemory(&mSendOverlappedEx, sizeof(stOverlappedEx));
}

ClientInfo::ClientInfo(const UINT32 id)
	:mId(id)
{
	ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
	ZeroMemory(&mSendOverlappedEx, sizeof(stOverlappedEx));
}

bool ClientInfo::IsSending()
{
	std::lock_guard<std::mutex> guard(mSendingLock);
	return m_bSending;
}

void ClientInfo::SetSending(const bool bSending)
{
	std::lock_guard<std::mutex> guard(mSendingLock);
	m_bSending = bSending;
}

bool ClientInfo::IsConnecting()
{
	std::lock_guard<std::mutex> guard(mIsConnectingLock);
	return mIsConnecting;
}

void ClientInfo::CloseSocket()
{
	closesocket(GetClientSocket());
	SetLatestClosedTimeSec(GetCurTimeSec());
	SetClientSocket(INVALID_SOCKET);
}

void ClientInfo::AsyncAccept(SOCKET listenSocket)
{
	if (IsConnecting())
	{
		return;
	}
	
	UINT64 curTimeSec = GetCurTimeSec();
	if (curTimeSec < GetLatestClosedTimeSec())
	{
		return;
	}

	UINT64 diff = curTimeSec - GetLatestClosedTimeSec();
	if (diff <= RE_USE_SESSION_WAIT_TIMESEC)
	{
		return;
	}
		
	PostAccept(listenSocket);
}

void ClientInfo::SetIsConnecting(const bool isConnecting)
{
	std::lock_guard<std::mutex> guard(mIsConnectingLock);
	mIsConnecting = isConnecting;
}

stPacket ClientInfo::GetRecvPacket()
{
	std::lock_guard<std::mutex> guard(mRecvPacketPoolLock);
	if (mRecvPacketPool.empty())
	{
		return stPacket();
	}
	
	stPacket p = mRecvPacketPool.front();
	mRecvPacketPool.pop();
	return p;
}

std::optional<stPacket> ClientInfo::GetSendPacket()
{
	std::lock_guard<std::mutex> guard(mSendPacketPoolLock);
	if (mSendPacketPool.empty())
	{
		return std::nullopt;
	}

	return mSendPacketPool.front();
}

void ClientInfo::PopSendPacketPool()
{
	std::lock_guard<std::mutex> guard(mSendPacketPoolLock);
	if (false == mSendPacketPool.empty())
	{
		mSendPacketPool.pop_front();
	}	
}

void ClientInfo::AddRecvPacket(const stPacket& packet)
{
	std::lock_guard<std::mutex> guard(mRecvPacketPoolLock);
	mRecvPacketPool.push(packet);
}

void ClientInfo::AddSendPacket(const stPacket& packet)
{
	std::lock_guard<std::mutex> guard(mSendPacketPoolLock);
	mSendPacketPool.push_back(packet);
}

bool ClientInfo::PostAccept(const SOCKET listenSocket)
{
	mLatestClosedTimeSec = UINT64_MAX;

	mClientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == mClientSocket)
	{
		printf_s("client Socket WSASocket Error : %d\n", GetLastError());
		return false;
	}

	DWORD bytes = 0;
	mAcceptOverlappedEx.m_eOperation = IOOperation::ACCEPT;
	mAcceptOverlappedEx.m_clientId = mId;
	
	if (FALSE == AcceptEx(
		listenSocket,
		mClientSocket,
		mAcceptBuf, 0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&bytes,
		reinterpret_cast<LPOVERLAPPED>(&mAcceptOverlappedEx.m_wsaOverlapped)
	))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			printf_s("AcceptEx Error : %d\n", GetLastError());
			return false;
		}
	}

	return true;
}