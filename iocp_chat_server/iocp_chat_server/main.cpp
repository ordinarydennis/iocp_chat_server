#include "ChatServer.h"

int main()
{
	ChatServer chatServer;

	Error error = Error::NONE;
	error = chatServer.Init();
	if (Error::NONE != error)
	{
		printf("[ERROR] Error Number: %d, Get Last Error: %d\n", error, WSAGetLastError());
		return static_cast<int>(error);
	}

	chatServer.Run();
	chatServer.Destroy();

	return static_cast<int>(Error::NONE);
}

