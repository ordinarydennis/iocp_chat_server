#include "ChatServer.h"
#include "../thirdparty/flags.h"
#include <iostream>

int main(int argc, char* argv[])
{
	const flags::args args(argc, argv);

	const auto port = args.get<int>("port");
	if (!port)
	{
		std::cerr << "No Port." << std::endl;
		return static_cast<int>(Error::PORT);
	}

	ChatServer chatServer;

	Error error = Error::NONE;
	error = chatServer.Init(*port);
	if (Error::NONE != error)
	{
		printf("[ERROR] Error Number: %d, Get Last Error: %d\n", error, WSAGetLastError());
		return static_cast<int>(error);
	}

	chatServer.Run();
	chatServer.Destroy();

	return static_cast<int>(Error::NONE);
}

