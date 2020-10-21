#include "ClientInfo.h"
#include "Common.h"
#include "Logger.h"
#include <Windows.h>
#include <chrono>
#include <mswsock.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

namespace JNet
{
	using namespace std::chrono;

	ClientInfo::ClientInfo()
	{
		Init();
	}

	ClientInfo::ClientInfo(const ClientInfo& clientInfo)
		:mId(clientInfo.mId)
	{
		Init();
	}

	ClientInfo::ClientInfo(const UINT32 id)
		:mId(id)
	{
		Init();
	}

	void ClientInfo::Init()
	{
		ZeroMemory(&mAcceptOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&mSendOverlappedEx, sizeof(stOverlappedEx));

		mRecvBuffer = new char[PACKET_DATA_BUFFER_SIZE];
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
		//TODO 읽을 땐 lock 없어도 되는건가?
		return mIsSending;
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
		InterlockedExchange(&mIsSending, bSending);
	}

	void ClientInfo::AddSendPacket(const JCommon::stPacket& packet)
	{
		std::lock_guard<std::mutex> guard(mSendPacketPoolLock);
		mSendPacketPool.push_back(packet);
	}

	void ClientInfo::AsyncAccept(SOCKET listenSocket)
	{
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

	void ClientInfo::SetRecvPacketBuff(const char* pData, const size_t dataSize)
	{
		if ((mRecvPacketWPos + dataSize) >= PACKET_DATA_BUFFER_SIZE)
		{
			auto remainDataSize = mRecvPacketWPos - mRecvPacketRPos;

			if (remainDataSize > 0)
			{
				CopyMemory(&mRecvBuffer[0], &mRecvBuffer[mRecvPacketRPos], remainDataSize);
				mRecvPacketWPos = remainDataSize;
			}
			else
			{
				mRecvPacketWPos = 0;
			}

			mRecvPacketRPos = 0;
		}

		memcpy_s(&mRecvBuffer[mRecvPacketWPos], dataSize, pData, dataSize);
		mRecvPacketWPos += static_cast<UINT32>(dataSize);
	}

	std::optional<JCommon::stPacket> ClientInfo::GetPacket()
	{
		//TODO mRecvPacketWPos, mRecvPacketRPos Lock 필요 할 듯?
		UINT32 remainByte = mRecvPacketWPos - mRecvPacketRPos;

		if (remainByte < JCommon::PACKET_HEADER_SIZE)
		{
			return std::nullopt;
		}

		auto pHeader = (JCommon::stPacketHeader*)&mRecvBuffer[mRecvPacketRPos];

		if (pHeader->mSize > remainByte)
		{
			return std::nullopt;
		}

		JCommon::stPacket packet;
		packet.mHeader.mPacket_id = pHeader->mPacket_id;
		packet.mHeader.mSize = pHeader->mSize;
		packet.mClientFrom = mId;
		//TODO 불필요한 복사 개선하기
		memcpy_s(packet.mBody, pHeader->mSize, &mRecvBuffer[JCommon::PACKET_HEADER_SIZE + mRecvPacketRPos], pHeader->mSize);

		mRecvPacketRPos += pHeader->mSize;

		return packet;
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
			JCommon::Logger::Error("Client Socket WSASocket Error : %d", GetLastError());
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
				JCommon::Logger::Error("AcceptEx Error : %d", GetLastError());
				return false;
			}
		}

		return true;
	}
}