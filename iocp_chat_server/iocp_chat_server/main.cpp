//#include "ChatServer.h"
#include "../thirdparty/flags.h"

#pragma comment(lib,"JChat.lib")
#include "Service.h"
#include "../thirdparty/JChat/JChat/Define.h"
#include "CrushDump.h"

int main(int argc, char* argv[])
{
	InitCrashDump(1);

	const flags::args args(argc, argv);

	const auto port = args.get<UINT16>("port");
	if (!port)
	{
		return static_cast<int>(JCommon::ERROR_CODE::PORT);
	}


	JCommon::ERROR_CODE errorCode = JCommon::ERROR_CODE::NONE;


	JChat::ServiceArgs serviceArgs;

	serviceArgs.mPort = *port;
	serviceArgs.mMaxRoomCount = 10;

	JChat::Service service;
	//서버 설정 클래스를 만들어서 넘겨주자
	errorCode = service.Init(serviceArgs);
	if (JCommon::ERROR_CODE::NONE != errorCode)
	{
		printf("[ERROR] Error Number: %d, Get Last Error: %d\n", errorCode, WSAGetLastError());
		return static_cast<int>(errorCode);
	}

	service.Run();
	service.Destroy();

	return static_cast<int>(JCommon::ERROR_CODE::NONE);

}

