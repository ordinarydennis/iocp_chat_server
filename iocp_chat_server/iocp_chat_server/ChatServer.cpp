#include "ChatServer.h"
#include "ServerConfig.h"
#include "RedisConfig.h"
#include "Packet.h"
#include "RedisPacket.h"
#include "RedisTask.h"
#include "Packet.h"
#include <string>
#include <iostream>

Error ChatServer::Init()
{
	Error error = Error::NONE;

	mNetwork = std::make_unique<Network>();
	error = mNetwork->Init(SERVER_PORT);
	if (Error::NONE != error)
		return error;
	
	mRedis = std::make_unique<Redis>();
	mRedis->Connect(REDIS_IP, REDIS_PORT);

	RegisterRecvProc();

	return error;
}
void ChatServer::RegisterRecvProc()
{
	mRecvPacketProcDict = std::unordered_map<PacketID, receiver>();
	mRecvPacketProcDict[PacketID::DEV_ECHO] = &ChatServer::ProcEcho;
	mRecvPacketProcDict[PacketID::LOGIN_REQ] = &ChatServer::ProcLogin;
	mRecvPacketProcDict[PacketID::ROOM_ENTER_REQ] = &ChatServer::ProcRoomEnter;
	mRecvPacketProcDict[PacketID::ROOM_CHAT_REQ] = &ChatServer::ProcRoomChat;
	mRecvPacketProcDict[PacketID::ROOM_LEAVE_REQ] = &ChatServer::ProcRoomLeave;
}
void ChatServer::ProcEcho(stPacket packet)
{
	ClientInfo* clientInfo = mNetwork->GetClientInfo(packet.mClientFrom);
	if (nullptr == clientInfo)
	{
		return;
	}

	packet.mClientTo = packet.mClientFrom;
	clientInfo->AddSendPacket(packet);
	mNetwork->AddToClientPoolSendPacket(clientInfo);
}
void ChatServer::ProcLogin(stPacket packet)
{
	LoginReqPacket loginReqPacket;
	loginReqPacket.SetPacket(packet.mBody);
	printf("Login User Id : %s passwd : %s \n", loginReqPacket.GetUserId(), loginReqPacket.GetUserPw());

	char buf[MAX_USER_ID_BYTE_LENGTH + MAX_USER_PW_BYTE_LENGTH] = { 0, };
	memcpy_s(buf, strlen(loginReqPacket.GetUserId()), loginReqPacket.GetUserId(), strlen(loginReqPacket.GetUserId()));
	memcpy_s(&buf[MAX_USER_ID_BYTE_LENGTH], strlen(loginReqPacket.GetUserPw()), loginReqPacket.GetUserPw(), strlen(loginReqPacket.GetUserPw()));
	LoginReqRedisPacket redisReqPacket(packet.mClientFrom, REDIS_TASK_ID::REQUEST_LOGIN, buf, MAX_USER_ID_BYTE_LENGTH + MAX_USER_PW_BYTE_LENGTH);
	mRedis->RequestTask(redisReqPacket.GetTask());
}
void ChatServer::ProcRoomEnter(stPacket packet)
{
	RoomEnterReqPacket reqPacket(packet.mBody, packet.GetBodySize());

	UINT32 roomNumber = 0;
	if (mRoomManager.IsExistRoom(reqPacket.GetRoomNumber()))
	{
		roomNumber = reqPacket.GetRoomNumber();
	}
	else
	{
		roomNumber = mRoomManager.CreateRoom();
	}

	ChatUser* chatUser = mChatUserManager.GetUser(packet.mClientFrom);
	chatUser->SetRoom(mRoomManager.GetRoom(roomNumber));
	mRoomManager.EnterRoom(roomNumber, chatUser);

	//입장 결과 반환
	ERROR_CODE result = ERROR_CODE::NONE;
	size_t bodySize = sizeof(result);
	char body[MAX_SOCKBUF] = { 0, };
	memcpy_s(body, bodySize, &result, bodySize);
	
	SendPacket(
		packet.mClientFrom, 
		packet.mClientFrom, 
		static_cast<UINT16>(PacketID::ROOM_ENTER_RES),
		body, 
		bodySize
	);

	Room* room = mRoomManager.GetRoom(roomNumber);

	//방 유저 리스트 내려주기
	auto userList = room->GetUserList();
	char userListBuf[MAX_SOCKBUF] = { 0, };
	size_t userListBufSize = 0;
	UINT16 userCount = static_cast<UINT16>(userList->size());	//자기 자신은 카운트 하지 않음.
	memcpy_s(userListBuf, 1, &userCount, 1);
	userListBufSize++;
	
	for (auto user : *userList)
	{
		UINT64 userUniqueId = user->GetClientId();
		
		const size_t userUniqueIdSize = sizeof(userUniqueId);
		const size_t bodySize = userUniqueIdSize + MAX_USER_ID_BYTE_LENGTH;

		char body[bodySize] = { 0, };
		size_t idLen = user->GetUserId().length();
		memcpy_s(body, userUniqueIdSize, &userUniqueId, userUniqueIdSize);
		memcpy_s(&body[userUniqueIdSize], 1, &idLen, 1);
		memcpy_s(&body[userUniqueIdSize + 1], user->GetUserId().length(), user->GetUserId().c_str(), user->GetUserId().length());
		
		size_t userDataSize = userUniqueIdSize + 1 + user->GetUserId().length();
		memcpy_s(&userListBuf[userListBufSize], userDataSize, body, userDataSize);
		userListBufSize += userDataSize;
	}

	if (0 < userCount)
	{
		SendPacket(
			packet.mClientFrom,
			packet.mClientFrom,
			static_cast<UINT16>(PacketID::ROOM_USER_LIST_NTF),
			userListBuf,
			userListBufSize
		);
	}

	//방 유저들에게 노티
	UINT64 newUserUniqueId = chatUser->GetClientId();
	const size_t userUniqueIdSize = sizeof(newUserUniqueId);
	const size_t bodySize2 = userUniqueIdSize + MAX_USER_ID_BYTE_LENGTH;
	char body2[bodySize2] = { 0, };
	size_t idLen = chatUser->GetUserId().length();
	memcpy_s(body2, userUniqueIdSize, &newUserUniqueId, userUniqueIdSize);
	memcpy_s(&body2[userUniqueIdSize], 1, &idLen, 1);
	memcpy_s(&body2[userUniqueIdSize + 1], chatUser->GetUserId().length(), chatUser->GetUserId().c_str(), chatUser->GetUserId().length());

	for (auto user : *userList)
	{
		if (user->GetClientId() == newUserUniqueId)
			continue;

		SendPacket(
			newUserUniqueId,
			user->GetClientId(),
			static_cast<UINT16>(PacketID::ROOM_NEW_USER_NTF),
			body2,
			bodySize2
		);
	}
}
void ChatServer::SendPacket(UINT32 from, UINT32 to, UINT16 packetId, char* body, size_t bodySize)
{
	stPacketHeader header;
	header.mSize = static_cast<UINT16>(bodySize + PACKET_HEADER_SIZE);
	header.mPacket_id = packetId;

	stPacket chatPacket(
		from,
		to,
		header,
		body,
		bodySize
	);

	ClientInfo* clientInfo = mNetwork->GetClientInfo(to);
	clientInfo->AddSendPacket(chatPacket);
	mNetwork->AddToClientPoolSendPacket(clientInfo);
}

void ChatServer::ProcRoomChat(stPacket packet)
{
	const ChatUser* chatUser = mChatUserManager.GetUser(packet.mClientFrom);
	
	Room* room = chatUser->GetRoom();
	if (nullptr == room)
	{
		return;
	}

	ERROR_CODE result = ERROR_CODE::NONE;
	const size_t bodySize = sizeof(result);
	char body[bodySize] = {0,};
	memcpy_s(body, bodySize, &result, bodySize);

	SendPacket(
		packet.mClientFrom,
		packet.mClientFrom,
		static_cast<UINT16>(PacketID::ROOM_CHAT_RES),
		body,
		bodySize
	);

	auto userList = room->GetUserList();
	for (auto user : *userList)
	{
		char chat[MAX_CHAT_MSG_SIZE] = { 0, };
		size_t chatLenth = strlen(packet.mBody);
		memcpy_s(&chat, chatLenth, packet.mBody, chatLenth);

		//RoomChatReqPacket reqPacket(packet.mBody);
		const size_t bodySize = MAX_USER_ID_BYTE_LENGTH + MAX_CHAT_MSG_SIZE;
		char body[bodySize] = {0,};
		memcpy_s(body, chatUser->GetUserId().length(), chatUser->GetUserId().c_str(), chatUser->GetUserId().length());
		memcpy_s(&body[MAX_USER_ID_BYTE_LENGTH], chatLenth, chat, chatLenth);

		SendPacket(
			packet.mClientFrom,
			user->GetClientId(),
			static_cast<UINT16>(PacketID::ROOM_CHAT_NOTIFY),
			body,
			bodySize
		);
	}
}
void ChatServer::ProcRoomLeave(stPacket packet)
{
	ChatUser* chatUser = mChatUserManager.GetUser(packet.mClientFrom);

	Room* room = chatUser->GetRoom();
	if (nullptr == room)
	{
		return;
	}

	auto userList = room->GetUserList();
	for (auto user : *userList)
	{
		UINT64 userUniqueId = chatUser->GetClientId();
		const size_t bodySize  = sizeof(userUniqueId);
		char body[bodySize] = { 0, };
		memcpy_s(body, bodySize, &userUniqueId, bodySize);

		SendPacket(
			packet.mClientFrom,
			user->GetClientId(),
			static_cast<UINT16>(PacketID::ROOM_LEAVE_USER_NTF),
			body,
			bodySize
		);
	}

	room->RemoveUser(chatUser);

	ERROR_CODE result = ERROR_CODE::NONE;
	const size_t bodySize = sizeof(result);
	char body[bodySize] = { 0, };
	memcpy_s(body, bodySize, &result, bodySize);

	SendPacket(
		packet.mClientFrom,
		packet.mClientFrom,
		static_cast<UINT16>(PacketID::ROOM_LEAVE_RES),
		body,
		bodySize
	);
}
void ChatServer::Run()
{
	mNetwork->Run();
	mRedis->Run();
	
	//TODO 최흥배
	// SetSendPacketThread 스레드는 Network 관련 스레드로 Network 레이어로 들어가야 합니다.
	SetReceivePacketThread();
	SetRedisResponseThread();
	Waiting();
}
void ChatServer::Waiting()
{
	printf("아무 키나 누를 때까지 대기합니다\n");
	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}
}
void ChatServer::SetRedisResponseThread()
{
	mRedisResponseThread = std::thread([this]() { RedisResponseThread(); });
}
void ChatServer::RedisResponseThread()
{
	while (mRedisResponseRun)
	{
		RedisTask task = mRedis->GetResponseTask();

		if (REDIS_TASK_ID::INVALID == task.GetTaskId())
		{
			Sleep(50);
			continue;
		}

		if (REDIS_TASK_ID::RESPONSE_LOGIN == task.GetTaskId())
		{
			LoginResRedisPacket loginResRedisPacket(task.GetClientId(), task.GetTaskId(), task.GetData(), task.GetDataSize());
			ERROR_CODE loginResult = loginResRedisPacket.GetResult();
			UINT16 bodySize = sizeof(loginResult);

			stPacket resPacket;
			resPacket.mClientFrom = task.GetClientId();
			resPacket.mClientTo = resPacket.mClientFrom;
			resPacket.mHeader.mSize = bodySize + PACKET_HEADER_SIZE;
			resPacket.mHeader.mPacket_id = static_cast<UINT16>(PacketID::LOGIN_RES);
			memcpy_s(resPacket.mBody, bodySize, &loginResult, bodySize);

			ClientInfo* clientInfo = mNetwork->GetClientInfo(resPacket.mClientTo);
			if (nullptr != clientInfo)
			{
				clientInfo->AddSendPacket(resPacket);
				mNetwork->AddToClientPoolSendPacket(clientInfo);
				if (ERROR_CODE::NONE == loginResRedisPacket.GetResult())
				{
					ChatUser chatUser(loginResRedisPacket.GetUserId(), clientInfo);
					mChatUserManager.AddUser(chatUser);
				}
			}
		}
	}
}
void ChatServer::SetReceivePacketThread()
{
	mReceivePacketThread = std::thread([this]() { ReceivePacketThread(); });
}
void ChatServer::ReceivePacketThread()
{
	while (mReceivePacketRun)
	{
		if (mNetwork->IsEmptyClientPoolRecvPacket())
		{
			Sleep(50);
			continue;
		}
		
		std::pair<ClientInfo*, size_t> recvedPacketInfo = mNetwork->GetClientRecvedPacket();
		ClientInfo* pClientInfo = recvedPacketInfo.first;
		size_t dwIoSize = recvedPacketInfo.second;

		stPacketHeader header;
		memcpy_s(&header.mSize, sizeof(UINT16), pClientInfo->GetRecvBuf(), sizeof(UINT16));
		memcpy_s(&header.mPacket_id, sizeof(UINT16), &pClientInfo->GetRecvBuf()[2], sizeof(UINT16));

		char body[MAX_SOCKBUF] = { 0, };
		UINT32 bodySize = (UINT32)dwIoSize - PACKET_HEADER_SIZE;
		memcpy_s(body, bodySize, &pClientInfo->GetRecvBuf()[PACKET_HEADER_SIZE], bodySize);

		ProcessPacket(stPacket(pClientInfo->GetId(), 0, header, body, bodySize));
	}
}
void ChatServer::ProcessPacket(stPacket p)
{
	PacketID packetId = static_cast<PacketID>(p.mHeader.mPacket_id);
	auto iter = mRecvPacketProcDict.find(packetId);
	if (iter != mRecvPacketProcDict.end())
	{
		(this->*(iter->second))(p);
	}
}
//void ChatServer::SetSendPacketThread()
//{
//	for (int i = 0; i < 1; i++)
//	{
//		mSendPacketThreads.emplace_back([this]() { SendPacketThread(); });
//	}
//}
//void ChatServer::SendPacketThread()
//{
//	while (mSendPacketRun)
//	{
//		if (mNetwork->IsEmptyClientPoolSendPacket())
//		{
//			Sleep(50);
//			continue;
//		}
//			
//		ClientInfo* clientInfo = mNetwork->GetClientSendingPacket();
//		stPacket p = clientInfo->GetSendPacket();
//		if (0 == p.mHeader.mSize)
//			continue;
//
//		while (clientInfo->IsSending())
//			Sleep(50);
//		
//		//TODO: 이거 위치 옮기자
//		clientInfo->SetLastSendPacket(p);
//		mNetwork->SendData(p);
//	}
//}
void ChatServer::Destroy()
{
	mReceivePacketRun = false;
	if (mReceivePacketThread.joinable())
	{
		mReceivePacketThread.join();
	}

	//mSendPacketRun = false;
	//for (auto& th : mSendPacketThreads)
	//{
	//	if (th.joinable())
	//	{
	//		th.join();
	//	}
	//}

	mNetwork->Destroy();
}