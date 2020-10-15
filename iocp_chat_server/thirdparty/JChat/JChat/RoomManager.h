#pragma once

#include "Room.h"
#include <basetsd.h>
#include <unordered_map>
#include <mutex>

namespace JChat
{
	class RoomManager
	{
	public:
		RoomManager() = default;

		~RoomManager() = default;

		bool IsExistRoom(const UINT32 roomNumber);

		Room* GetRoom(const UINT32 roomNumber);

		void EnterRoom(UINT32 roomNumber, ChatUser* chatUser);

	private:
		UINT32 CreateRoom();

	private:
		std::unordered_map<UINT32, Room>	mRoomDict;
		UINT32								mRoomCount = 0;
	};
}
