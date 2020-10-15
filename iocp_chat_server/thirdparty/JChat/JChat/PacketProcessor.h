#pragma once

#include "../../JCommon/JCommon/Common.h"
#include <basetsd.h>
#include <functional>
#include <thread>

namespace JNet
{
	class Network;
	class Redis;
	class RedisTask;
	enum class REDIS_TASK_ID : UINT16;
	struct stOverlappedEx;
}

namespace JCommon
{
	struct stPacket;
}

namespace JChat
{
	class ChatUserManager;

	class RoomManager;

	class PacketProcessor
	{
	public:
		PacketProcessor();

		~PacketProcessor();

		JCommon::ERROR_CODE Init(const UINT16 port);

		void            Run();

		void            ProcessPacket(const JNet::stOverlappedEx& recvOverlappedEx);

		void            ProcessRedisPacket(const JNet::RedisTask& task);

		void            ProcEcho(const JCommon::stPacket& packet);

		void            ProcLogin(const JCommon::stPacket& packet);

		void            ProcRoomEnter(const JCommon::stPacket& packet);

		void            ProcRoomChat(const JCommon::stPacket& packet);

		void            ProcRoomLeave(const JCommon::stPacket& packet);

		void            RedisProcLogin(const JNet::RedisTask& task);

		void            Destroy();

	private:
		void            ReceivePacketThread();

		void			RegisterRecvProc();

		void			SendPacket(const UINT32 from, const UINT32 to, const UINT16 packetId, const char* body, const size_t bodySize);


	private:
		JNet::Network*			mNetwork = nullptr;
		JNet::Redis*			mRedis = nullptr;
		ChatUserManager*		mChatUserManager = nullptr;
		RoomManager*			mRoomManager = nullptr;

		using receiver = void(PacketProcessor::*)(const JCommon::stPacket& p);
		std::unordered_map<JCommon::PACKET_ID, receiver>  mRecvPacketProcDict;

		using RedisReceiver = void(PacketProcessor::*)(const JNet::RedisTask& task);
		std::unordered_map<JNet::REDIS_TASK_ID, RedisReceiver>  mRecvRedisPacketProcDict;

		std::function<void(JCommon::stPacket)>           mPacketSender;

		std::thread	                            mReceivePacketThread;
		bool                                    mReceivePacketRun = true;
	};
}


