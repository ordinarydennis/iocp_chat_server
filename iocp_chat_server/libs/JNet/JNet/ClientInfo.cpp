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

	ClientInfo::~ClientInfo()
	{
		if (nullptr != mRecvPacketPool)
		{
			delete mRecvPacketPool;
			mRecvPacketPool = nullptr;
		}

		//if (nullptr != mRecvBuf)
		//{
		//	delete mRecvBuf;
		//	mRecvBuf = nullptr;
		//}

		//if (nullptr != mSendBuf)
		//{
		//	delete mSendBuf;
		//	mRecvBuf = nullptr;
		//}
	}

	void ClientInfo::Init()
	{
		ZeroMemory(&mAcceptOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&mSendOverlappedEx, sizeof(stOverlappedEx));	

		mRecvPacketPool = new char[RECV_PACKET_POOL_SIZE];
		ZeroMemory(mRecvPacketPool, RECV_PACKET_POOL_SIZE);
	}

	void ClientInfo::SetInfo(const UINT32 id, const size_t packetBuffSize)
	{
		mId = id;

		//TODO 버그
		//mRecvBuf = new char[packetBuffSize];
		//ZeroMemory(mRecvBuf, packetBuffSize);

		//mSendBuf = new char[packetBuffSize];
		//ZeroMemory(mSendBuf, packetBuffSize);
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
		//Reading interlocked variables
		return InterlockedExchangeAdd(&mIsSending, 0);
	}

	std::optional<JCommon::stPacket> ClientInfo::GetSendPacket()
	{
		auto entryPacket = mSendPacketQueue.Front();
		if (nullptr == entryPacket)
		{
			return std::nullopt;
		}

		return entryPacket->mPacket;
	}

	void ClientInfo::PopSendPacketPool()
	{
		mSendPacketQueue.Pop();
	}

	void ClientInfo::SetSending(const bool bSending)
	{
		InterlockedExchange(&mIsSending, bSending);
	}

	void ClientInfo::AddSendPacket(const JCommon::stPacket& packet)
	{
		JCommon::EntryPacket entryPacket;
		entryPacket.mPacket = packet;
		mSendPacketQueue.Push(entryPacket);
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

	std::optional<JCommon::stPacket> ClientInfo::RecvPacket(const char* pData, const size_t dataSize)
	{
		if ((mRecvPacketWPos + dataSize) >= RECV_PACKET_POOL_SIZE)
		{
			auto remainDataSize = mRecvPacketWPos - mRecvPacketRPos;

			if (remainDataSize > 0)
			{
				CopyMemory(&mRecvPacketPool[0], &mRecvPacketPool[mRecvPacketRPos], remainDataSize);
				mRecvPacketWPos = remainDataSize;
			}
			else
			{
				mRecvPacketWPos = 0;
			}

			mRecvPacketRPos = 0;
		}

		memcpy_s(&mRecvPacketPool[mRecvPacketWPos], dataSize, pData, dataSize);
		mRecvPacketWPos += static_cast<UINT32>(dataSize);

		UINT32 remainByte = mRecvPacketWPos - mRecvPacketRPos;

		if (remainByte < JCommon::PACKET_HEADER_SIZE)
		{
			return std::nullopt;
		}

		auto pHeader = (JCommon::stPacketHeader*)(&mRecvPacketPool[mRecvPacketRPos]);

		if (pHeader->mSize > remainByte)
		{
			return std::nullopt;
		}

		//TODO 패킷 크기 별로 분기
		JCommon::stPacket packet;
		packet.mHeader.mPacket_id = pHeader->mPacket_id;
		packet.mHeader.mSize = pHeader->mSize;
		packet.mClientFrom = mId;
		//TODO 불필요한 복사 개선하기
		size_t bodySize = pHeader->mSize - JCommon::PACKET_HEADER_SIZE;
		memcpy_s(
			packet.mBody, 
			bodySize,
			&mRecvPacketPool[JCommon::PACKET_HEADER_SIZE + mRecvPacketRPos], 
			bodySize
		);

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