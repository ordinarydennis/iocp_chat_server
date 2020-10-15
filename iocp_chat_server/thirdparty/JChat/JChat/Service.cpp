#include "Service.h"
#include "PacketProcessor.h"
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

	JCommon::ERROR_CODE Service::Init(const UINT16 port)
	{
		JCommon::ERROR_CODE errorCode = JCommon::ERROR_CODE::NONE;
		errorCode = mPacketProcessor->Init(port);
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
		printf("�ƹ� Ű�� ���� ������ ����մϴ�\n");
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