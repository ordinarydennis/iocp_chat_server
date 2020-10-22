#pragma comment(lib,"JChat.lib")

#include "Service.h"
#include "Define.h"
#include "CrushDump.h"
#include "Logger.h"

int main(int argc, char* argv[])
{
	JCommon::Logger::Info("Start Server");

	InitCrashDump();

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
		JCommon::Logger::Error("Error Number: %d, Get Last Error: %d", errorCode, WSAGetLastError());
		return static_cast<int>(errorCode);
	}

	service.Run();
	service.Destroy();

	JCommon::Logger::Info("Detroy Server");

	return static_cast<int>(JCommon::ERROR_CODE::NONE);
}

