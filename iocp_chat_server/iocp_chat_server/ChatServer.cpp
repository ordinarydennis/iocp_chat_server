#include "ChatServer.h"
#include "PacketProcessor.h"
#include <string>
#include <iostream>

ChatServer::ChatServer()
{
	mPacketProcessor = new PacketProcessor();
}

ChatServer::~ChatServer()
{
	delete mPacketProcessor;
	mPacketProcessor = nullptr;

	WSACleanup();
}

Error ChatServer::Init()
{
	Error error = Error::NONE;

	error = mPacketProcessor->Init();

	return error;
}

void ChatServer::Run()
{
	mPacketProcessor->Run();

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

void ChatServer::Destroy()
{
	mPacketProcessor->Destroy();
}