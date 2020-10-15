//#include "ChatServer.h"
#include "../thirdparty/flags.h"
//#include "CrushDump.h"

#pragma comment(lib,"JChat.lib")
#include "Service.h"

int main(int argc, char* argv[])
{
	const flags::args args(argc, argv);

	const auto port = args.get<UINT16>("port");
	if (!port)
	{
		return static_cast<int>(JCommon::ERROR_CODE::PORT);
	}

	JChat::Service service;

	JCommon::ERROR_CODE errorCode = JCommon::ERROR_CODE::NONE;
	errorCode = service.Init(*port);
	if (JCommon::ERROR_CODE::NONE != errorCode)
	{
		printf("[ERROR] Error Number: %d, Get Last Error: %d\n", errorCode, WSAGetLastError());
		return static_cast<int>(errorCode);
	}

	service.Run();
	service.Destroy();


	return 0;
	//InitCrashDump(1);

	//const flags::args args(argc, argv);

	//const auto port = args.get<UINT16>("port");
	//if (!port)
	//{
	//	return static_cast<int>(Error::PORT);
	//}

	//ChatServer chatServer;

	//Error error = Error::NONE;
	//error = chatServer.Init(*port);
	//if (Error::NONE != error)
	//{
	//	printf("[ERROR] Error Number: %d, Get Last Error: %d\n", error, WSAGetLastError());
	//	return static_cast<int>(error);
	//}

	//chatServer.Run();
	//chatServer.Destroy();

	//return static_cast<int>(Error::NONE);
}

