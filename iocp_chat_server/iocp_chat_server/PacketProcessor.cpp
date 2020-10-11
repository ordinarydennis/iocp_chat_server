#include "PacketProcessor.h"
#include "ClientInfo.h"
#include "Packet.h"
#include "RedisPacket.h"
#include "RoomManager.h"
#include "Redis.h"
#include "RedisConfig.h"
#include "ChatUserManager.h"

PacketProcessor::PacketProcessor()
{
	mRedis = new Redis;
	mRoomManager = new RoomManager;
	mChatUserManager = new ChatUserManager;
	mNetwork = new Network;
}

PacketProcessor::~PacketProcessor()
{
	delete mRedis;
	mRedis = nullptr;

	delete mRoomManager;
	mRoomManager = nullptr;

	delete mChatUserManager;
	mChatUserManager = nullptr;

	delete mNetwork;
	mNetwork = nullptr;
}

Error PacketProcessor::Init(const UINT16 port)
{
	Error error = Error::NONE;
	error = mNetwork->Init(port);
	if (Error::NONE != error)
	{
		return error;
	}

	error = mRedis->Connect(REDIS_IP, REDIS_PORT);
	if (Error::NONE != error)
	{
		return error;
	}

	RegisterRecvProc();

	mPacketSender = mNetwork->GetPacketSender();

	return error;
}

void PacketProcessor::RegisterRecvProc()
{
	mRecvPacketProcDict[PacketID::DEV_ECHO] = &PacketProcessor::ProcEcho;
	mRecvPacketProcDict[PacketID::LOGIN_REQ] = &PacketProcessor::ProcLogin;
	mRecvPacketProcDict[PacketID::ROOM_ENTER_REQ] = &PacketProcessor::ProcRoomEnter;
	mRecvPacketProcDict[PacketID::ROOM_CHAT_REQ] = &PacketProcessor::ProcRoomChat;
	mRecvPacketProcDict[PacketID::ROOM_LEAVE_REQ] = &PacketProcessor::ProcRoomLeave;

	mRecvRedisPacketProcDict[RedisTaskID::RESPONSE_LOGIN] = &PacketProcessor::RedisProcLogin;
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

void PacketProcessor::ProcessPacket(const stOverlappedEx& recvOverlappedEx)
{
	const char* recvBuf = recvOverlappedEx.m_wsaBuf.buf;
	const size_t recvBufSize = recvOverlappedEx.m_wsaBuf.len;
	UINT32 clientId = recvOverlappedEx.m_clientId;

	stPacketHeader header;
	memcpy_s(&header.mSize, sizeof(UINT16), recvBuf, sizeof(UINT16));
	memcpy_s(&header.mPacket_id, sizeof(UINT16), &recvBuf[2], sizeof(UINT16));

	char body[MAX_SOCKBUF] = { 0, };
	UINT32 bodySize = (UINT32)recvBufSize - PACKET_HEADER_SIZE;
	memcpy_s(body, bodySize, &recvBuf[PACKET_HEADER_SIZE], bodySize);

	stPacket packet(clientId, 0, header, body, bodySize);

	PacketID packetId = static_cast<PacketID>(packet.mHeader.mPacket_id);
	auto iter = mRecvPacketProcDict.find(packetId);
	if (iter != mRecvPacketProcDict.end())
	{
		(this->*(iter->second))(packet);
	}
}

void PacketProcessor::ProcessRedisPacket(const RedisTask& task)
{
	auto iter = mRecvRedisPacketProcDict.find(task.GetTaskId());
	if (iter != mRecvRedisPacketProcDict.end())
	{
		(this->*(iter->second))(task);
	}
}

void PacketProcessor::SendPacket(const UINT32 from, const UINT32 to, const UINT16 packetId, const char* body, const size_t bodySize)
{
	stPacketHeader header;
	header.mSize = static_cast<UINT16>(bodySize + PACKET_HEADER_SIZE);
	header.mPacket_id = packetId;

	mPacketSender
	(
		stPacket(
			from,
			to,
			header,
			body,
			bodySize
		)
	);
}

void PacketProcessor::ProcEcho(const stPacket& packet)
{
	stPacket resPacket = packet;
	resPacket.mClientTo = packet.mClientFrom;
	mPacketSender(resPacket);
}

void PacketProcessor::ProcLogin(const stPacket& packet)
{
	LoginReqPacket loginReqPacket;
	loginReqPacket.SetPacket(packet.mBody);
	
	printf("Login User Id : %s passwd : %s \n", loginReqPacket.GetUserId(), loginReqPacket.GetUserPw());

	LoginReqRedisPacket redisReqPacket;
	redisReqPacket.mClientId = packet.mClientFrom;
	redisReqPacket.mRedisTaskId = RedisTaskID::REQUEST_LOGIN;
	size_t userIdSize = strnlen_s(loginReqPacket.GetUserId(), MAX_USER_ID_BYTE_LENGTH);
	memcpy_s(redisReqPacket.mUserId, userIdSize, loginReqPacket.GetUserId(), userIdSize);
	size_t userPwSize = strnlen_s(loginReqPacket.GetUserPw(), MAX_USER_PW_BYTE_LENGTH);
	memcpy_s(redisReqPacket.mUserPw, userPwSize, loginReqPacket.GetUserPw(), userPwSize);
	
	mRedis->RequestTask(redisReqPacket.EncodeTask());
}

void PacketProcessor::ProcRoomEnter(const stPacket& packet)
{
	ChatUser* chatUser = mChatUserManager->GetUser(packet.mClientFrom);
	if (nullptr == chatUser)
	{
		return;
	}

	RoomEnterReqPacket reqPacket(packet.mBody, packet.GetBodySize());

	UINT32 roomNumber = reqPacket.GetRoomNumber();
	mRoomManager->EnterRoom(roomNumber, chatUser);
	chatUser->SetRoomNumber(roomNumber);

	ResultResPacket resultResPacket(ERROR_CODE::NONE);
	SendPacket(
		packet.mClientFrom,
		packet.mClientFrom,
		static_cast<UINT16>(PacketID::ROOM_ENTER_RES),
		resultResPacket.GetBody(),
		resultResPacket.GetBodySize()
	);

	//방 유저 리스트 내려주기
	Room* room = mRoomManager->GetRoom(roomNumber);
	auto userList = room->GetUserList();
	UINT16 userCount = static_cast<UINT16>(userList->size()) - 1; //자신은 제외
	if (0 < userCount)
	{
		RoomUserListNTFPacket roomUserListNTFPacket(chatUser->GetClientId(), *userList);
		SendPacket(
			packet.mClientFrom,
			packet.mClientFrom,
			static_cast<UINT16>(PacketID::ROOM_USER_LIST_NTF),
			roomUserListNTFPacket.GetBody(),
			roomUserListNTFPacket.GetBodySize()
		);
	}

	//방 유저들에게 노티
	RoomEnterNTFPacket roomEnterNTFPacket(chatUser->GetClientId(), chatUser->GetUserId());
	room->Notify(
		chatUser->GetClientId(),
		static_cast<UINT16>(PacketID::ROOM_NEW_USER_NTF),
		roomEnterNTFPacket.GetBody(),
		roomEnterNTFPacket.GetBodySize(),
		mPacketSender
	);
}

void PacketProcessor::ProcRoomChat(const stPacket& packet)
{
	const ChatUser* chatUser = mChatUserManager->GetUser(packet.mClientFrom);
	Room* room = mRoomManager->GetRoom(chatUser->GetRoomNumber());
	if (nullptr == chatUser || nullptr == room)
	{
		return;
	}

	RoomChatReqPacket roomChatReqPacket(chatUser->GetUserId(), packet.mBody, strnlen_s(packet.mBody, MAX_SOCKBUF));
	room->Notify(
		packet.mClientFrom,
		static_cast<UINT16>(PacketID::ROOM_CHAT_NOTIFY),
		roomChatReqPacket.GetBody(),
		roomChatReqPacket.GetBodySize(),
		mPacketSender
	);

	ResultResPacket resultResPacket(ERROR_CODE::NONE);
	SendPacket(
		packet.mClientFrom,
		packet.mClientFrom,
		static_cast<UINT16>(PacketID::ROOM_CHAT_RES),
		resultResPacket.GetBody(),
		resultResPacket.GetBodySize()
	);
}

void PacketProcessor::ProcRoomLeave(const stPacket& packet)
{
	ChatUser* chatUser = mChatUserManager->GetUser(packet.mClientFrom);
	Room* room = mRoomManager->GetRoom(chatUser->GetRoomNumber());
	if (nullptr == chatUser || nullptr == room)
	{
		return;
	}

	RoomLeaveNTFPacket roomLeaveNTFPacket(chatUser->GetClientId());

	room->Notify(
		packet.mClientFrom,
		static_cast<UINT16>(PacketID::ROOM_LEAVE_USER_NTF),
		roomLeaveNTFPacket.GetBody(),
		roomLeaveNTFPacket.GetBodySize(),
		mPacketSender
	);

	room->RemoveUser(chatUser);

	ResultResPacket resultResPacket(ERROR_CODE::NONE);
	SendPacket(
		packet.mClientFrom,
		packet.mClientFrom,
		static_cast<UINT16>(PacketID::ROOM_LEAVE_RES),
		resultResPacket.GetBody(),
		resultResPacket.GetBodySize()
	);
}

void PacketProcessor::RedisProcLogin(const RedisTask& task)
{
	LoginResRedisPacket loginResRedisPacket;
	loginResRedisPacket.DecodeTask(task);
	
	ResultResPacket resultResPacket(loginResRedisPacket.mResult);
	
	SendPacket(
		task.GetClientId(),
		task.GetClientId(),
		static_cast<UINT16>(PacketID::LOGIN_RES),
		resultResPacket.GetBody(),
		resultResPacket.GetBodySize()
	);

	if (ERROR_CODE::NONE == loginResRedisPacket.mResult)
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