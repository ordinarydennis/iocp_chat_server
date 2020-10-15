#include "ClientInfo.h"
#include "Common.h"
#include <Windows.h>
#include <chrono>
#include <mswsock.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

namespace JNet
{
	using namespace std::chrono;

	ClientInfo::ClientInfo(const ClientInfo& clientInfo)
		:mId(clientInfo.mId)
	{
		ZeroMemory(&mAcceptOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&mSendOverlappedEx, sizeof(stOverlappedEx));
	}

	ClientInfo::ClientInfo(const UINT32 id)
		:mId(id)
	{
		ZeroMemory(&mAcceptOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&mSendOverlappedEx, sizeof(stOverlappedEx));
	}

	void ClientInfo::SetId(const UINT32 id)
	{
		mId = id;
	}

	UINT32 ClientInfo::GetId() const
	{ 
		return mId; 
	};

	stOverlappedEx* ClientInfo::GetRecvOverlappedEx()
	{ 
		return &mRecvOverlappedEx; 
	}

	stOverlappedEx* ClientInfo::GetSendOverlappedEx()
	{ 
		return &mSendOverlappedEx; 
	}

	char* ClientInfo::GetRecvBuf()
	{ 
		return mRecvBuf; 
	}

	char* ClientInfo::GetSendBuf()
	{ 
		return mSendBuf; 
	}

	SOCKET ClientInfo::GetClientSocket() const 
	{ 
		return mClientSocket; 
	}

	void ClientInfo::SetClientSocket(const SOCKET clientSocket)
	{ 
		mClientSocket = clientSocket; 
	};

	void ClientInfo::CloseSocket()
	{
		closesocket(GetClientSocket());
		SetLatestClosedTimeSec(JCommon::GetCurTimeSec());
		SetClientSocket(INVALID_SOCKET);
	}

	void ClientInfo::SetLatestClosedTimeSec(const UINT64 latestClosedTimeSec)
	{ 
		mLatestClosedTimeSec = latestClosedTimeSec; 
	};

	bool ClientInfo::IsSending()
	{
		std::lock_guard<std::mutex> guard(mSendingLock);
		return m_bSending;
	}

	std::optional<JCommon::stPacket> ClientInfo::GetSendPacket()
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

	void ClientInfo::SetSending(const bool bSending)
	{
		std::lock_guard<std::mutex> guard(mSendingLock);
		m_bSending = bSending;
	}

	void ClientInfo::AddSendPacket(const JCommon::stPacket& packet)
	{
		std::lock_guard<std::mutex> guard(mSendPacketPoolLock);
		mSendPacketPool.push_back(packet);
	}

	void ClientInfo::AsyncAccept(SOCKET listenSocket)
	{
		if (IsConnecting())
		{
			return;
		}

		UINT64 curTimeSec = JCommon::GetCurTimeSec();
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

	bool ClientInfo::IsConnecting()
	{
		std::lock_guard<std::mutex> guard(mIsConnectingLock);
		return mIsConnecting;
	}

	UINT64 ClientInfo::GetLatestClosedTimeSec() const 
	{ 
		return mLatestClosedTimeSec; 
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
}