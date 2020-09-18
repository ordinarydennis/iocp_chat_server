#include "ChatServer.h"

int main()
{
	ChatServer chatServer;

	chatServer.Init();
	chatServer.Run();
	chatServer.Destroy();

	return static_cast<int>(Error::NONE);
}

