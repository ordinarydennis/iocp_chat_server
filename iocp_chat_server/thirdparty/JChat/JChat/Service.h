#pragma once

#include "../../JCommon/JCommon/Common.h"
#include <basetsd.h>

namespace JChat
{
	class PacketProcessor;

	struct ServiceArgs;

	class Service
	{
	public:
		Service();
		
		~Service();

		JCommon::ERROR_CODE Init(const ServiceArgs& args);

		void Run();

		void Destroy();

	private:
		void Waiting();

	private:
		PacketProcessor* mPacketProcessor = nullptr;
	};
}