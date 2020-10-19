#pragma once

#include "Room.h"
#include <basetsd.h>
#include <mutex>
#include <vector>

namespace JChat
{
	class RoomManager
	{
	public:
		RoomManager() = default;

		~RoomManager() = default;

		void						Init(const UINT32 roomStartIndex, const UINT32 maxRoomUserCount);

		Room*						GetRoom(const UINT32 roomNumber);

		JCommon::CLIENT_ERROR_CODE	EnterRoom(UINT32 roomNumber, ChatUser* chatUser);

	private:
		void						CreateRooms();

	private:
		static const UINT32			MAX_ROOM_COUNT = 5;

	private:
		std::vector<Room>			mRooms;
		UINT32						mMaxRoomUserCount = 0;
		UINT32						mRoomStartIndex = 0;
	};
}
