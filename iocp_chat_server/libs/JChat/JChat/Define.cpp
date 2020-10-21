#include "Define.h"
#include "Common.h"
#include "../../../thirdparty/flags.h"
#include "Logger.h"

namespace JChat
{
	std::optional<ServiceArgs> DecodeServiceArgs(int argc, char* argv[])
	{
		const flags::args args(argc, argv);

		const auto port = args.get<UINT16>("port");
		if (!port)
		{
			JCommon::Logger::Error("Invalide Service Argument, error code: %d", JCommon::ERROR_CODE::ARGS_PORT);
			return std::nullopt;
		}

		const auto maxClientCount = args.get<UINT16>("max_client_count");
		if (!maxClientCount)
		{
			JCommon::Logger::Error("Invalide Service Argument, error code: %d", JCommon::ERROR_CODE::ARGS_MAX_CLINENT_COUNT);
			return std::nullopt;
		}

		const auto roomStartIndex = args.get<UINT16>("room_start_index");
		if (!roomStartIndex)
		{
			JCommon::Logger::Error("Invalide Service Argument, error code: %d", JCommon::ERROR_CODE::ARGS_PORT_ROOM_START_INDEX);
			return std::nullopt;
		}

		const auto roomMaxUserCount = args.get<UINT16>("room_max_user_count");
		if (!roomMaxUserCount)
		{
			JCommon::Logger::Error("Invalide Service Argument, error code: %d", JCommon::ERROR_CODE::ARGS_PORT_ROOM_MAX_USER_COUNT);
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