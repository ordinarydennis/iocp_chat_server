#pragma once

#include "../../JCommon/JCommon/Common.h"
#include <basetsd.h>

namespace JChat
{
	class PacketProcessor;

	class Service
	{
	public:
		Service();
		
		~Service();

		JCommon::ERROR_CODE Init(const UINT16 port);

		void Run();

		void Destroy();

	private:
		void Waiting();

	private:
		PacketProcessor* mPacketProcessor = nullptr;
	};
}