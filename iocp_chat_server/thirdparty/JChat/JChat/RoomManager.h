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

		void Init(const UINT32 maxRoomCount);

		bool IsExistRoom(const UINT32 roomNumber);

		Room* GetRoom(const UINT32 roomNumber);

		bool EnterRoom(UINT32 roomNumber, ChatUser* chatUser);

	private:
		void CreateRooms(const UINT32 maxRoomCount);

	private:
		std::unordered_map<UINT32, Room>	mRoomDict;
		UINT32								mRoomCount = 0;

		std::vector<Room>					mRooms;
		UINT32								mMaxRoomCount = 0;
	};
}
