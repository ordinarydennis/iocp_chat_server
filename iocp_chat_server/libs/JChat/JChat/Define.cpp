#include "Define.h"
#include "../../../thirdparty/flags.h"

namespace JChat
{
	std::optional<ServiceArgs> DecodeServiceArgs(int argc, char* argv[])
	{
		const flags::args args(argc, argv);

		const auto port = args.get<UINT16>("port");
		if (!port)
		{
			//TODO 로그
			//return static_cast<int>(JCommon::ERROR_CODE::ARGS_PORT);
			return std::nullopt;
		}

		const auto maxClientCount = args.get<UINT16>("max_client_count");
		if (!maxClientCount)
		{
			//todo: 로그 남기기
			//return static_cast<int>(JCommon::ERROR_CODE::ARGS_MAX_CLINENT_COUNT);
			return std::nullopt;
		}

		const auto roomStartIndex = args.get<UINT16>("room_start_index");
		if (!roomStartIndex)
		{
			//TODO 로그
			//return static_cast<int>(JCommon::ERROR_CODE::ARGS_PORT_ROOM_START_INDEX);
			return std::nullopt;
		}

		const auto roomMaxUserCount = args.get<UINT16>("room_max_user_count");
		if (!roomMaxUserCount)
		{
			//TODO 로그
			//return static_cast<int>(JCommon::ERROR_CODE::ARGS_PORT_ROOM_MAX_USER_COUNT);
			return std::nullopt;
		}

		ServiceArgs serviceArgs;
		serviceArgs.mPort = *port;
		serviceArgs.mMaxClientCount = *maxClientCount;
		serviceArgs.mRoomStartIndex = *roomStartIndex;
		serviceArgs.mMaxRoomUserCount = *roomMaxUserCount;

		return serviceArgs;
	}
}