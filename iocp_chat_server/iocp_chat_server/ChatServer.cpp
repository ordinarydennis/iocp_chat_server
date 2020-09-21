#include "ChatServer.h"

#include "ServerConfig.h"
#include <string>
#include <iostream>

void ChatServer::Init()
{
	mNetwork = std::make_unique<Network>();
	//네트워크 초기화
	mNetwork->Init();
}
void ChatServer::Run()
{
	mNetwork->Run();
	SetReceivePacketThread();
	SetSendPacketThread();
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
void ChatServer::SetReceivePacketThread()
{
	mReceivePacketThread = std::thread([this]() { ReceivePacketThread(); });
}
void ChatServer::ReceivePacketThread()
{
	while (mReceivePacketRun)
	{
		if (mNetwork->IsEmptyClientPoolRecvPacket())
			continue;

		stClientInfo* clientInfo = mNetwork->GetClientRecvedPacket();

		if (clientInfo->mRecvPacketPool.empty())
			continue;

		stPacket p = clientInfo->mRecvPacketPool.front();
		clientInfo->mRecvPacketPool.pop();

		if (0 == p.mHeader.mSize)
			continue;

		PacketID packetId = static_cast<PacketID>(p.mHeader.mPacket_id);
		switch (packetId)
		{
		case PacketID::DEV_ECHO:
			p.mClientTo = p.mClientFrom;
			clientInfo->mSendPacketPool.push(p);
			mNetwork->AddToClientPoolSendPacket(clientInfo);
			break;
		}
	}
}
void ChatServer::SetSendPacketThread()
{
	//mSendPacketThread = std::thread([this]() { SendPacketThread(); });
	//적절한 쓰래드 수는?
	for (int i = 0; i < 1; i++)
	{
		mSendPacketThreads.emplace_back([this]() { SendPacketThread(); });
	}
}
void ChatServer::SendPacketThread()
{
	while (mSendPacketRun)
	{
		if (mNetwork->IsEmptyClientPoolSendPacket())
			continue;

		//TODO: lock
		stClientInfo* c = mNetwork->GetClientSendingPacket();
		
		if (c->mSendPacketPool.empty())
			continue;

		//TODO: 전송중인 패킷이 있는지 확인
		//Sleep 말고 다른 방식으로 개선하자 개선하자
		while (c->m_bSending)
		{
			Sleep(50);
		};

		stPacket p = c->mSendPacketPool.front();
		c->mSendPacketPool.pop();
		c->mLastSendPacket = p;
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
	//if (mSendPacketThread.joinable())
	//{
	//	mSendPacketThread.join();
	//}
	for (auto& th : mSendPacketThreads)
	{
		if (th.joinable())
		{
			th.join();
		}
	}

	mNetwork->Destroy();
}