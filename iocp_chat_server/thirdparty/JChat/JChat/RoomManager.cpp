#include "RoomManager.h"

namespace JChat
{
	void RoomManager::Init(const UINT32 maxRoomCount)
	{
		mMaxRoomCount = maxRoomCount;
		CreateRooms(mMaxRoomCount);
	}

	bool RoomManager::IsExistRoom(const UINT32 roomNumber)
	{
		auto item = mRoomDict.find(roomNumber);
		return (item != mRoomDict.end()) ? true : false;
	}

	Room* RoomManager::GetRoom(const UINT32 roomNumber)
	{
		if (false == IsExistRoom(roomNumber))
		{
			return nullptr;
		}

		return &mRoomDict[roomNumber];
	}

	void RoomManager::CreateRooms(const UINT32 maxRoomCount)
	{
		for (UINT32 i = 0; i < maxRoomCount; i++)
		{
			mRooms.emplace_back();
			mRooms[i].SetRoomNumber(i);
		}
	}

	bool RoomManager::EnterRoom(UINT32 roomNumber, ChatUser* chatUser)
	{
		bool isEnter = false;
		if (roomNumber < mMaxRoomCount)
		{
			mRoomDict[roomNumber].AddUser(chatUser);
			isEnter = true;
		}

		return isEnter;
	}
}