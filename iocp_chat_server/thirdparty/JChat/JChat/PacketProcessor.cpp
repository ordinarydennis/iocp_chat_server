#include "PacketProcessor.h"
#include "ChatUserManager.h"
#include "RoomManager.h"
#include "Define.h"
#include "../../JNet/JNet/Network.h"
#include "../../JNet/JNet/Redis.h"
#include "../../JNet/JNet/RedisConfig.h"

namespace JChat
{
	PacketProcessor::PacketProcessor()
	{
		mNetwork = new JNet::Network;
		mRedis = new JNet::Redis;
		mChatUserManager = new ChatUserManager;
		mRoomManager = new RoomManager;
	}

	PacketProcessor::~PacketProcessor()
	{
		delete mNetwork;
		mNetwork = nullptr;

		delete mRedis;
		mRedis = nullptr;

		delete mChatUserManager;
		mChatUserManager = nullptr;
		
		delete mRoomManager;
		mRoomManager = nullptr;
	}

	JCommon::ERROR_CODE PacketProcessor::Init(const ServiceArgs& args)
	{
		JCommon::ERROR_CODE errorCode = JCommon::ERROR_CODE::NONE;
		errorCode = mNetwork->Init(args.mMaxClientCount, args.mPort);
		if (JCommon::ERROR_CODE::NONE != errorCode)
		{
			return errorCode;
		}

		errorCode = mRedis->Connect(JNet::REDIS_IP, JNet::REDIS_PORT);
		if (JCommon::ERROR_CODE::NONE != errorCode)
		{
			return errorCode;
		}

		mRoomManager->Init(args.mRoomStartIndex, args.mMaxRoomUserCount);

		RegisterRecvProc();

		mPacketSender = mNetwork->GetPacketSender();

		return errorCode;
	}

	void PacketProcessor::Run()
	{
		mReceivePacketThread = std::thread([this]() { ReceivePacketThread(); });

		mNetwork->Run();
		mRedis->Run();
	}

	void PacketProcessor::ReceivePacketThread()
	{
		while (mReceivePacketRun)
		{
			bool isPacket = false;

			auto recvedPacketInfoOpt = mNetwork->GetClientRecvedPacket();
			if (std::nullopt != recvedPacketInfoOpt)
			{
				isPacket = true;
				ProcessPacket(recvedPacketInfoOpt.value());
			}

			auto taskOpt = mRedis->GetResponseTask();
			if (std::nullopt != taskOpt)
			{
				isPacket = true;
				ProcessRedisPacket(taskOpt.value());
			}

			if (false == isPacket)
			{
				Sleep(1);
			}
		}
	}

	void PacketProcessor::ProcessPacket(const JNet::stOverlappedEx& recvOverlappedEx)
	{
		const char* recvBuf = recvOverlappedEx.m_wsaBuf.buf;
		const size_t recvBufSize = recvOverlappedEx.m_wsaBuf.len;
		UINT32 clientId = recvOverlappedEx.m_clientId;

		JCommon::stPacketHeader header;
		memcpy_s(&header.mSize, sizeof(UINT16), recvBuf, sizeof(UINT16));
		memcpy_s(&header.mPacket_id, sizeof(UINT16), &recvBuf[2], sizeof(UINT16));

		char body[JCommon::MAX_SOCKBUF] = { 0, };
		UINT32 bodySize = (UINT32)recvBufSize - JCommon::PACKET_HEADER_SIZE;
		memcpy_s(body, bodySize, &recvBuf[JCommon::PACKET_HEADER_SIZE], bodySize);

		JCommon::stPacket packet(clientId, 0, header, body, bodySize);

		JCommon::PACKET_ID packetId = static_cast<JCommon::PACKET_ID>(packet.mHeader.mPacket_id);
		auto iter = mRecvPacketProcDict.find(packetId);
		if (iter != mRecvPacketProcDict.end())
		{
			(this->*(iter->second))(packet);
		}
	}

	void PacketProcessor::ProcessRedisPacket(const JNet::RedisTask& task)
	{
		auto iter = mRecvRedisPacketProcDict.find(task.GetTaskId());
		if (iter != mRecvRedisPacketProcDict.end())
		{
			(this->*(iter->second))(task);
		}
	}

	void PacketProcessor::RegisterRecvProc()
	{
		mRecvPacketProcDict[JCommon::PACKET_ID::DEV_ECHO] = &PacketProcessor::ProcEcho;
		mRecvPacketProcDict[JCommon::PACKET_ID::LOGIN_REQ] = &PacketProcessor::ProcLogin;
		mRecvPacketProcDict[JCommon::PACKET_ID::ROOM_ENTER_REQ] = &PacketProcessor::ProcRoomEnter;
		mRecvPacketProcDict[JCommon::PACKET_ID::ROOM_CHAT_REQ] = &PacketProcessor::ProcRoomChat;
		mRecvPacketProcDict[JCommon::PACKET_ID::ROOM_LEAVE_REQ] = &PacketProcessor::ProcRoomLeave;

		mRecvRedisPacketProcDict[JNet::REDIS_TASK_ID::RESPONSE_LOGIN] = &PacketProcessor::RedisProcLogin;
	}

	void PacketProcessor::SendPacket(const UINT32 from, const UINT32 to, const UINT16 packetId, const char* body, const size_t bodySize)
	{
		JCommon::stPacketHeader header;
		header.mSize = static_cast<UINT16>(bodySize + JCommon::PACKET_HEADER_SIZE);
		header.mPacket_id = packetId;

		mPacketSender
		(
			JCommon::stPacket(
				from,
				to,
				header,
				body,
				bodySize
			)
		);
	}

	void PacketProcessor::Destroy()
	{
		mReceivePacketRun = false;
		if (mReceivePacketThread.joinable())
		{
			mReceivePacketThread.join();
		}

		mNetwork->Destroy();
		mRedis->Destroy();
	}
}