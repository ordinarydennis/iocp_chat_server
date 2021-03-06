#pragma once

#include "Common.h"
#include <basetsd.h>
#include <functional>
#include <thread>

namespace JNet
{
	class Network;
	class Redis;
	enum class REDIS_TASK_ID : UINT16;
	struct stOverlappedEx;
	struct RedisTask;
}

namespace JCommon
{
	struct stPacket;
}

namespace JChat
{
	class ChatUser;

	class ChatUserManager;

	class RoomManager;

	struct ServiceArgs;

	class RoomUser;

	class Room;

	class PacketProcessor
	{
	public:
		PacketProcessor();

		~PacketProcessor();

		JCommon::ERROR_CODE Init(const ServiceArgs& arg);

		void            Run();

		void            ProcessPacket(const JCommon::stPacket& packet);

		void            ProcessRedisPacket(const JNet::RedisTask& task);

		void            ProcEcho(const JCommon::stPacket& packet);

		void            ProcLogin(const JCommon::stPacket& packet);

		void            ProcRoomEnter(const JCommon::stPacket& packet);

		void            ProcRoomChat(const JCommon::stPacket& packet);

		void            ProcRoomLeave(const JCommon::stPacket& packet);

		void			ProcCloseSocket(const JCommon::stPacket& packet);

		void            RedisProcLogin(const JNet::RedisTask& task);
		
		ChatUser*		CheckUserAndSendError(UINT32 clientId);

		bool			EnterRoomAndSendError(UINT32 roomNumber, const RoomUser& roomUser);
		
		void			SendRoomUserList(Room* room, UINT32 recvClientId);

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
		std::unordered_map<JCommon::PACKET_ID, receiver>		mRecvPacketProcDict;

		using RedisReceiver = void(PacketProcessor::*)(const JNet::RedisTask& task);
		std::unordered_map<JNet::REDIS_TASK_ID, RedisReceiver>  mRecvRedisPacketProcDict;

		std::function<void(JCommon::stPacket)>					mPacketSender;

		std::thread	                            mReceivePacketThread;
		bool                                    mReceivePacketRun = true;
	};
}


