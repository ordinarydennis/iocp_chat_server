#include "ChatServer.h"
#include "ServerConfig.h"
#include "RedisConfig.h"
#include "Packet.h"
#include "RedisPacket.h"
#include "RedisTask.h"
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
	//패킷 파싱
	LoginReqPacket loginReqPacket;
	loginReqPacket.SetPacket(packet.mBody);
	printf("Login User Id : %s passwd : %s\\n", loginReqPacket.GetUserId(), loginReqPacket.GetUserPw());

	//레디스 로그인 처리
	LoginReqRedisPacket redisReqPacket(packet.mClientFrom, REDIS_TASK_ID::REQUEST_LOGIN, loginReqPacket.GetUserPw(), strlen(loginReqPacket.GetUserPw()));
	mRedis->RequestTask(redisReqPacket.GetTask());
}
void ChatServer::ProcRoonEnter(stPacket packet)
{
	//room number


	//방이 있으면 방 유저 리스트에 추가 
	//방이 없으면 새로운 방을 만들고 추가 

	//유
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