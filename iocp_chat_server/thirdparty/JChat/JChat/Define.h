#pragma once

#include <basetsd.h>
#include <optional>

namespace JChat
{
	struct ServiceArgs
	{
		UINT16 mPort = 0;
		UINT32 mMaxClientCount = 0;
		UINT32 mRoomStartIndex = 0;
		UINT32 mMaxRoomUserCount = 0;;
	};

	std::optional<ServiceArgs> DecodeServiceArgs(int argc, char* argv[]);
}
