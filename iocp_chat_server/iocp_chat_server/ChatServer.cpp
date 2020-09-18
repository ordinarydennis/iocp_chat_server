#include "ChatServer.h"
#include "Network.h"
#include "ServerConfig.h"
#include <string>
#include <iostream>

void ChatServer::Init()
{
	//네트워크 초기화
	NetworkInstance.Init();
}
void ChatServer::Run()
{
	NetworkInstance.Run();
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
		if (NetworkInstance.IsReceivePacketPoolEmpty())
			continue;

		stPacket p = NetworkInstance.ReceivePacket();

		if (0 == p.mHeader.mSize)
		{
			printf("User : %d Disconnection\n", p.mClientId);
			continue;
		}

		PacketID packetId = static_cast<PacketID>(p.mHeader.mPacket_id);
		switch (packetId)
		{
		case PacketID::DEV_ECHO:
			NetworkInstance.AddPacket(p);
			break;

		}
	}
}
void ChatServer::SetSendPacketThread()
{
	mSendPacketThread = std::thread([this]() { SendPacketThread(); });
}
void ChatServer::SendPacketThread()
{
	while (mSendPacketRun)
	{
		if (NetworkInstance.IsSendPacketPoolEmpty())
			continue;

		stPacket p = NetworkInstance.GetSendPacket();
		NetworkInstance.SendData(p.mClientId, p.mBody, strlen(p.mBody));
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
	if (mSendPacketThread.joinable())
	{
		mSendPacketThread.join();
	}

	NetworkInstance.Destroy();
}