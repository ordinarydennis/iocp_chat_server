#include "Service.h"
#include "PacketProcessor.h"
#include "Define.h"
#include "Logger.h"
#include <iostream>
#include <sstream>

namespace JChat
{
	Service::Service()
	{
		mPacketProcessor = new PacketProcessor;
	}

	Service::~Service()
	{
		delete mPacketProcessor;
		mPacketProcessor = nullptr;
	}

	JCommon::ERROR_CODE Service::Init(const ServiceArgs& args)
	{
		JCommon::ERROR_CODE errorCode = JCommon::ERROR_CODE::NONE;
		errorCode = mPacketProcessor->Init(args);
		return errorCode;
	}

	void Service::Run()
	{
		mPacketProcessor->Run();
		Waiting();
	}

	void Service::Destroy()
	{
		mPacketProcessor->Destroy();
	}

	void Service::Waiting()
	{
		JCommon::Logger::Info("아무 키나 누를 때까지 대기합니다");
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
}