#pragma once

#include "Room.h"
#include <basetsd.h>
#include <unordered_map>
#include <mutex>

class RoomManager
{
public:
	RoomManager();

	~RoomManager();
	
	bool IsExistRoom(UINT32 roomNumber);

	Room* GetRoom(UINT32 roomNumber);
	
	void EnterRoom(UINT32 roomNumber, ChatUser* chatUser);

private:
	UINT32 CreateRoom();

private:
	std::unordered_map<UINT32, Room>	mRoomDict;
	UINT32								mRoomCount = 0;
};

