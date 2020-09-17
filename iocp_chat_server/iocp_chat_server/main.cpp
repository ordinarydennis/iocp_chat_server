#include "ChatServer.h"

int main()
{
	try
	{
		ChatServerInstance.Init();
		ChatServerInstance.Run();
	}
	catch (Error error)
	{
		return static_cast<int>(error);
	}

	ChatServerInstance.Destroy();

	return static_cast<int>(Error::NONE);
}

