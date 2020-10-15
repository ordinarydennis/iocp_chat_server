#include "PacketProcessor.h"
#include "ChatUserManager.h"
#include "ChatUser.h"
#include "RoomManager.h"
#include "../../JNet/JNet/Network.h"
#include "../../JNet/JNet/Redis.h"
#include "../../JNet/JNet/RedisConfig.h"
#include "../../JNet/JNet/RedisPacket.h"
#include "../../JCommon/JCommon/Packet.h"

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

	JCommon::ERROR_CODE PacketProcessor::Init(const UINT16 port)
	{
		JCommon::ERROR_CODE errorCode = JCommon::ERROR_CODE::NONE;
		errorCode = mNetwork->Init(port);
		if (JCommon::ERROR_CODE::NONE != errorCode)
		{
			return errorCode;
		}

		errorCode = mRedis->Connect(JNet::REDIS_IP, JNet::REDIS_PORT);
		if (JCommon::ERROR_CODE::NONE != errorCode)
		{
			return errorCode;
		}

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

	void PacketProcessor::ProcEcho(const JCommon::stPacket& packet)
	{
		JCommon::stPacket resPacket = packet;
		resPacket.mClientTo = packet.mClientFrom;
		mPacketSender(resPacket);
	}

	void PacketProcessor::ProcLogin(const JCommon::stPacket& packet)
	{
		JCommon::LoginReqPacket loginReqPacket;
		loginReqPacket.SetPacket(packet.mBody);

		printf("Login User Id : %s passwd : %s \n", loginReqPacket.GetUserId(), loginReqPacket.GetUserPw());

		JNet::LoginReqRedisPacket redisReqPacket;
		redisReqPacket.mClientId = packet.mClientFrom;
		redisReqPacket.mRedisTaskId = JNet::REDIS_TASK_ID::REQUEST_LOGIN;
		size_t userIdSize = strnlen_s(loginReqPacket.GetUserId(), JCommon::MAX_USER_ID_BYTE_LENGTH);
		memcpy_s(redisReqPacket.mUserId, userIdSize, loginReqPacket.GetUserId(), userIdSize);
		size_t userPwSize = strnlen_s(loginReqPacket.GetUserPw(), JCommon::MAX_USER_PW_BYTE_LENGTH);
		memcpy_s(redisReqPacket.mUserPw, userPwSize, loginReqPacket.GetUserPw(), userPwSize);

		mRedis->RequestTask(redisReqPacket.EncodeTask());
	}

	void PacketProcessor::ProcRoomEnter(const JCommon::stPacket& packet)
	{
		ChatUser* chatUser = mChatUserManager->GetUser(packet.mClientFrom);
		if (nullptr == chatUser)
		{
			return;
		}

		JCommon::RoomEnterReqPacket reqPacket(packet.mBody, packet.GetBodySize());

		UINT32 roomNumber = reqPacket.GetRoomNumber();
		mRoomManager->EnterRoom(roomNumber, chatUser);
		chatUser->SetRoomNumber(roomNumber);

		JCommon::ResultResPacket resultResPacket(JCommon::CLIENT_ERROR_CODE::NONE);
		SendPacket(
			packet.mClientFrom,
			packet.mClientFrom,
			static_cast<UINT16>(JCommon::PACKET_ID::ROOM_ENTER_RES),
			resultResPacket.GetBody(),
			resultResPacket.GetBodySize()
		);

		//방 유저 리스트 내려주기
		Room* room = mRoomManager->GetRoom(roomNumber);
		auto userList = room->GetUserList();
		UINT16 userCount = static_cast<UINT16>(userList->size()) - 1; //자신은 제외
		if (0 < userCount)
		{
			//TODO RoomUserListNTFPacket가 charUser를 알아야 하는데 이부분 수정 할 것
			//JCommon::RoomUserListNTFPacket roomUserListNTFPacket(chatUser->GetClientId(), *userList);
			//SendPacket(
			//	packet.mClientFrom,
			//	packet.mClientFrom,
			//	static_cast<UINT16>(PACKET_ID::ROOM_USER_LIST_NTF),
			//	roomUserListNTFPacket.GetBody(),
			//	roomUserListNTFPacket.GetBodySize()
			//);
		}

		//방 유저들에게 노티
		JCommon::RoomEnterNTFPacket roomEnterNTFPacket(chatUser->GetClientId(), chatUser->GetUserId());
		room->Notify(
			chatUser->GetClientId(),
			static_cast<UINT16>(JCommon::PACKET_ID::ROOM_NEW_USER_NTF),
			roomEnterNTFPacket.GetBody(),
			roomEnterNTFPacket.GetBodySize(),
			mPacketSender
		);
	}

	void PacketProcessor::ProcRoomChat(const JCommon::stPacket& packet)
	{
		const ChatUser* chatUser = mChatUserManager->GetUser(packet.mClientFrom);
		Room* room = mRoomManager->GetRoom(chatUser->GetRoomNumber());
		if (nullptr == chatUser || nullptr == room)
		{
			return;
		}

		JCommon::RoomChatReqPacket roomChatReqPacket(chatUser->GetUserId(), packet.mBody, strnlen_s(packet.mBody, JCommon::MAX_SOCKBUF));
		room->Notify(
			packet.mClientFrom,
			static_cast<UINT16>(JCommon::PACKET_ID::ROOM_CHAT_NOTIFY),
			roomChatReqPacket.GetBody(),
			roomChatReqPacket.GetBodySize(),
			mPacketSender
		);

		JCommon::ResultResPacket resultResPacket(JCommon::CLIENT_ERROR_CODE::NONE);
		SendPacket(
			packet.mClientFrom,
			packet.mClientFrom,
			static_cast<UINT16>(JCommon::PACKET_ID::ROOM_CHAT_RES),
			resultResPacket.GetBody(),
			resultResPacket.GetBodySize()
		);
	}

	void PacketProcessor::ProcRoomLeave(const JCommon::stPacket& packet)
	{
		ChatUser* chatUser = mChatUserManager->GetUser(packet.mClientFrom);
		Room* room = mRoomManager->GetRoom(chatUser->GetRoomNumber());
		if (nullptr == chatUser || nullptr == room)
		{
			return;
		}

		JCommon::RoomLeaveNTFPacket roomLeaveNTFPacket(chatUser->GetClientId());

		room->Notify(
			packet.mClientFrom,
			static_cast<UINT16>(JCommon::PACKET_ID::ROOM_LEAVE_USER_NTF),
			roomLeaveNTFPacket.GetBody(),
			roomLeaveNTFPacket.GetBodySize(),
			mPacketSender
		);

		room->RemoveUser(chatUser);

		JCommon::ResultResPacket resultResPacket(JCommon::CLIENT_ERROR_CODE::NONE);
		SendPacket(
			packet.mClientFrom,
			packet.mClientFrom,
			static_cast<UINT16>(JCommon::PACKET_ID::ROOM_LEAVE_RES),
			resultResPacket.GetBody(),
			resultResPacket.GetBodySize()
		);
	}

	void PacketProcessor::RedisProcLogin(const JNet::RedisTask& task)
	{
		JNet::LoginResRedisPacket loginResRedisPacket;
		loginResRedisPacket.DecodeTask(task);

		JCommon::ResultResPacket resultResPacket(loginResRedisPacket.mResult);

		SendPacket(
			task.GetClientId(),
			task.GetClientId(),
			static_cast<UINT16>(JCommon::PACKET_ID::LOGIN_RES),
			resultResPacket.GetBody(),
			resultResPacket.GetBodySize()
		);

		if (JCommon::CLIENT_ERROR_CODE::NONE == loginResRedisPacket.mResult)
		{
			ChatUser chatUser(loginResRedisPacket.mUserId, task.GetClientId());
			mChatUserManager->AddUser(chatUser);
		}
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