#include "RoomManager.h"

RoomManager::RoomManager()
{

}

RoomManager::~RoomManager()
{

}

Room* RoomManager::GetRoom(UINT32 roomNumber)
{
	if (false == IsExistRoom(roomNumber))
	{
		return nullptr;
	}

	return &mRoomDict[roomNumber];
}

bool RoomManager::IsExistRoom(UINT32 roomNumber)
{
	auto item = mRoomDict.find(roomNumber);
	return (item != mRoomDict.end()) ? true : false;
}

UINT32 RoomManager::CreateRoom()
{
	Room room;
	room.SetRoomNumber(mRoomCount);
	mRoomDict[mRoomCount] = room;
	UINT32 roomNumber = mRoomCount;
	mRoomCount++;
	return roomNumber;
}

void RoomManager::EnterRoom(UINT32 roomNumber, ChatUser* chatUser)
{
	mRoomDict[roomNumber].AddUser(chatUser);
}