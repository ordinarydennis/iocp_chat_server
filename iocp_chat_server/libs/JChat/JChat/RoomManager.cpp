#include "RoomManager.h"

namespace JChat
{
	void RoomManager::Init(const UINT32 roomStartIndex, const UINT32 maxRoomUserCount)
	{
		mRoomStartIndex = roomStartIndex;
		mMaxRoomUserCount = maxRoomUserCount;
		CreateRooms();
	}

	Room* RoomManager::GetRoom(const UINT32 roomNumber)
	{
		UINT32 roomIndex = roomNumber - mRoomStartIndex;

		if (roomIndex >= RoomManager::MAX_ROOM_COUNT)
		{
			return nullptr;
		}

		return &mRooms[roomIndex];
	}

	void RoomManager::CreateRooms()
	{
		for (UINT32 i = 0; i < RoomManager::MAX_ROOM_COUNT; i++)
		{
			mRooms.emplace_back();
			mRooms[i].SetRoomNumber(mRoomStartIndex + i);
		}
	}

	JCommon::CLIENT_ERROR_CODE RoomManager::EnterRoom(UINT32 roomNumber,const RoomUser& roomUser)
	{	
		UINT32 roomIndex = roomNumber - mRoomStartIndex;
		
		if (roomIndex >= RoomManager::MAX_ROOM_COUNT)
		{
			return JCommon::CLIENT_ERROR_CODE::INVALID_ROOM_NUMBER;
		}

		if (mRooms[roomIndex].GetUserList()->size() >= mMaxRoomUserCount)
		{
			return JCommon::CLIENT_ERROR_CODE::FULL_ROOM_USER;
		}

		mRooms[roomIndex].AddUser(roomUser);

		return JCommon::CLIENT_ERROR_CODE::NONE;
	}
}