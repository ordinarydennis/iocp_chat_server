#include "ChatServer.h"
#include "ServerConfig.h"
#include "RedisConfig.h"
#include "Packet.h"
#include "RedisPacket.h"
#include "RedisTask.h"
#include "Packet.h"
#include <string>
#include <iostream>

void ChatServer::Init()
{
	mNetwork = std::make_unique<Network>();
	mNetwork->Init(SERVER_PORT);
	
	mRedis = std::make_unique<Redis>();
	mRedis->Connect(REDIS_IP, REDIS_PORT);

	RegisterRecvProc();
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
	printf("Login User Id : %s passwd : %s\\n", loginReqPacket.GetUserId(), loginReqPacket.GetUserPw());

	char buf[MAX_USER_ID_BYTE_LENGTH + MAX_USER_PW_BYTE_LENGTH] = { 0, };

	memcpy_s(buf, MAX_USER_ID_BYTE_LENGTH, loginReqPacket.GetUserId(), MAX_USER_ID_BYTE_LENGTH);
	memcpy_s(&buf[MAX_USER_ID_BYTE_LENGTH], MAX_USER_PW_BYTE_LENGTH, loginReqPacket.GetUserPw(), MAX_USER_PW_BYTE_LENGTH);
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
	
	//이미 존재 하는 유저이면 실패
	chatUser->SetRoom(mRoomManager.GetRoom(roomNumber));
	mRoomManager.EnterRoom(roomNumber, chatUser);

	//입장 결과 반환
	stPacketHeader Header;
	Header.mPacket_id = static_cast<UINT16>(PacketID::ROOM_ENTER_RES);
	
	RoomEnterResPacket resPacket;
	resPacket.SetResult(ERROR_CODE::NONE);
	ERROR_CODE result = resPacket.GetResult();
	size_t bodySize = sizeof(result);
	Header.mSize = static_cast<UINT16>(bodySize + PACKET_HEADER_SIZE);

	char body[128] = { 0, };
	memcpy_s(body, bodySize, &result, bodySize);
	
	stPacket chatPacket(
		packet.mClientFrom,
		packet.mClientFrom,
		Header,
		body,
		bodySize
	);


	chatUser->GetClientInfo()->AddSendPacket(chatPacket);
	mNetwork->AddToClientPoolSendPacket(chatUser->GetClientInfo());

	Room* room = mRoomManager.GetRoom(roomNumber);

	auto  userList = room->GetUserList();
	for (auto user : *userList)
	{
		UINT64 userUniqueId = chatUser->GetClientId();
		const size_t userUniqueIdSize = sizeof(userUniqueId);
		const size_t bodySize = userUniqueIdSize + MAX_USER_ID_BYTE_LENGTH;
		
		char body[bodySize] = { 0, };
		size_t idLen = chatUser->GetUserId().length();
		memcpy_s(body, userUniqueIdSize, &userUniqueId, userUniqueIdSize);
		memcpy_s(&body[userUniqueIdSize], 1, &idLen, 1);
		memcpy_s(&body[userUniqueIdSize + 1], chatUser->GetUserId().length(), chatUser->GetUserId().c_str(), chatUser->GetUserId().length());

		stPacketHeader Header;
		Header.mPacket_id = static_cast<UINT16>(PacketID::ROOM_NEW_USER_NTF);
		Header.mSize = static_cast<UINT16>(bodySize + PACKET_HEADER_SIZE);
		stPacket chatPacket(	
			packet.mClientFrom,
			user->GetClientId(),
			Header,
			body,
			bodySize
		);

		user->GetClientInfo()->AddSendPacket(chatPacket);
		mNetwork->AddToClientPoolSendPacket(user->GetClientInfo());
	}
}
void ChatServer::ProcRoomChat(stPacket packet)
{
	const ChatUser* chatUser = mChatUserManager.GetUser(packet.mClientFrom);
	
	Room* room = chatUser->GetRoom();
	if (nullptr == room)
	{
		return;
	}

	stPacketHeader Header;
	Header.mPacket_id = static_cast<UINT16>(PacketID::ROOM_CHAT_RES);

	RoomChatResPacket resPacket;
	resPacket.SetResult(ERROR_CODE::NONE);
	ERROR_CODE result = resPacket.GetResult();
	size_t bodySize = sizeof(result);
	Header.mSize = static_cast<UINT16>(bodySize + PACKET_HEADER_SIZE);

	char body[128] = {0,};
	memcpy_s(body, bodySize, &result, bodySize);

	stPacket chatPacket(
		packet.mClientFrom,
		packet.mClientFrom,
		Header,
		body,
		bodySize
	);

	ClientInfo* clientInfo = mNetwork->GetClientInfo(packet.mClientFrom);
	clientInfo->AddSendPacket(chatPacket);
	mNetwork->AddToClientPoolSendPacket(clientInfo);

	auto userList = room->GetUserList();
	for (auto user : *userList)
	{
		RoomChatReqPacket reqPacket(packet.mBody);
		const size_t bodySize = MAX_USER_ID_BYTE_LENGTH + MAX_CHAT_MSG_SIZE;
		char body[bodySize] = {0,};
		memcpy_s(body, chatUser->GetUserId().length(), chatUser->GetUserId().c_str(), chatUser->GetUserId().length());
		memcpy_s(&body[MAX_USER_ID_BYTE_LENGTH], strlen(reqPacket.GetChat()), reqPacket.GetChat(), strlen(reqPacket.GetChat()));

		stPacketHeader Header;
		Header.mPacket_id = static_cast<UINT16>(PacketID::ROOM_CHAT_NOTIFY);
		Header.mSize = static_cast<UINT16>(bodySize + PACKET_HEADER_SIZE);
		stPacket chatPacket(
			packet.mClientFrom,
			user->GetClientId(),
			Header,
			body,
			bodySize
		);

		user->GetClientInfo()->AddSendPacket(chatPacket);
		mNetwork->AddToClientPoolSendPacket(user->GetClientInfo());
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

	room->RemoveUser(chatUser);

	stPacketHeader Header;
	Header.mPacket_id = static_cast<UINT16>(PacketID::ROOM_LEAVE_RES);

	RoomChatResPacket resPacket;
	resPacket.SetResult(ERROR_CODE::NONE);
	ERROR_CODE result = resPacket.GetResult();
	size_t bodySize = sizeof(result);
	Header.mSize = static_cast<UINT16>(bodySize + PACKET_HEADER_SIZE);

	//TODO: 128 수정
	char body[128] = { 0, };
	memcpy_s(body, bodySize, &result, bodySize);

	stPacket chatPacket(
		packet.mClientFrom,
		packet.mClientFrom,
		Header,
		body,
		bodySize
	);

	chatUser->GetClientInfo()->AddSendPacket(chatPacket);
	mNetwork->AddToClientPoolSendPacket(chatUser->GetClientInfo());

	auto userList = room->GetUserList();
	for (auto user : *userList)
	{
		UINT64 userUniqueId = chatUser->GetClientId();
		const size_t bodySize  = sizeof(userUniqueId);
		char body[bodySize] = { 0, };
		memcpy_s(body, bodySize, &userUniqueId, bodySize);

		stPacketHeader Header;
		Header.mPacket_id = static_cast<UINT16>(PacketID::ROOM_LEAVE_USER_NTF);
		Header.mSize = static_cast<UINT16>(bodySize + PACKET_HEADER_SIZE);
		stPacket chatPacket(
			packet.mClientFrom,
			user->GetClientId(),
			Header,
			body,
			bodySize
		);

		user->GetClientInfo()->AddSendPacket(chatPacket);
		mNetwork->AddToClientPoolSendPacket(user->GetClientInfo());
	}
}
void ChatServer::Run()
{
	mNetwork->Run();
	mRedis->Run();
	SetSendPacketThread();
	SetReceivePacketThread();
	SetSendPacketThread();
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
			
			LoginResPacket loginResPacket;
			loginResPacket.SetResult(loginResRedisPacket.GetResult());

			stPacket resPacket;
			resPacket.mClientFrom = task.GetClientId();
			resPacket.mClientTo = resPacket.mClientFrom;
			resPacket.mHeader.mPacket_id = static_cast<UINT16>(PacketID::LOGIN_RES);

			ERROR_CODE loginResult = loginResPacket.GetResult();
			UINT16 bodySize = sizeof(loginResult);
			resPacket.mHeader.mSize = bodySize + PACKET_HEADER_SIZE;

			memcpy_s(resPacket.mBody, bodySize, &loginResult, bodySize);
			ClientInfo* clientInfo = mNetwork->GetClientInfo(resPacket.mClientTo);
			if (nullptr != clientInfo)
			{
				clientInfo->AddSendPacket(resPacket);
				mNetwork->AddToClientPoolSendPacket(clientInfo);
			}
		
			if (ERROR_CODE::NONE == loginResRedisPacket.GetResult())
			{
				//todo: lock
				ChatUser chatUser(loginResRedisPacket.GetUserId(), clientInfo);
				mChatUserManager.AddUser(chatUser);
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
			//TODO: sleep
			continue;
		}
		
		ClientInfo* clientInfo = mNetwork->GetClientRecvedPacket();

		stPacket p = clientInfo->GetRecvPacket();
		if (0 == p.mHeader.mSize)
			continue;

		ProcessPacket(p);
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
void ChatServer::SetSendPacketThread()
{
	for (int i = 0; i < 2; i++)
	{
		mSendPacketThreads.emplace_back([this]() { SendPacketThread(); });
	}
}
void ChatServer::SendPacketThread()
{
	while (mSendPacketRun)
	{
		if (mNetwork->IsEmptyClientPoolSendPacket())
		{
			//sleep
			continue;
		}
			
		ClientInfo* c = mNetwork->GetClientSendingPacket();
		
		stPacket p = c->GetSendPacket();
		if (0 == p.mHeader.mSize)
		{
			continue;
		}

		//TODO: 전송중인 패킷이 있는지 확인
		//Sleep 말고 다른 방식으로 개선하자 개선하자
		while (c->IsSending())
		{
			Sleep(50);
		};

		//TODO: 이거 위치 옮기자
		c->SetLastSendPacket(p);
		mNetwork->SendData(p);
	}
}
void ChatServer::Destroy()
{
	//해제 순서 중요한가?
	mReceivePacketRun = false;
	if (mReceivePacketThread.joinable())
	{
		mReceivePacketThread.join();
	}

	mSendPacketRun = false;
	for (auto& th : mSendPacketThreads)
	{
		if (th.joinable())
		{
			th.join();
		}
	}

	mNetwork->Destroy();
}