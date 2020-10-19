#pragma comment(lib,"JChat.lib")

#include "Service.h"
#include "Define.h"
#include "CrushDump.h"

int main(int argc, char* argv[])
{
	InitCrashDump(1);

	auto serviceArgsOpt = JChat::DecodeServiceArgs(argc, argv);
	if (std::nullopt == serviceArgsOpt)
	{
		return static_cast<int>(JCommon::ERROR_CODE::ARGS_INVALID);
	}

	JChat::Service service;
	JCommon::ERROR_CODE errorCode = JCommon::ERROR_CODE::NONE;

	errorCode = service.Init(serviceArgsOpt.value());
	if (JCommon::ERROR_CODE::NONE != errorCode)
	{
		printf("[ERROR] Error Number: %d, Get Last Error: %d\n", errorCode, WSAGetLastError());
		return static_cast<int>(errorCode);
	}

	service.Run();
	service.Destroy();

	return static_cast<int>(JCommon::ERROR_CODE::NONE);
}

