#include "RoomManager.h"

RoomManager::RoomManager()
{

}

RoomManager::~RoomManager()
{

}

Room* RoomManager::GetRoom(UINT32 roomNumber)
{
	//std::lock_guard<std::mutex> guard(mRoomDictLock);
	if (false == IsExistRoom(roomNumber))
	{
		return nullptr;
	}

	return &mRoomDict[roomNumber];
}

bool RoomManager::IsExistRoom(UINT32 roomNumber)
{
	//std::lock_guard<std::mutex> guard(mRoomDictLock);
	auto item = mRoomDict.find(roomNumber);
	return (item != mRoomDict.end()) ? true : false;
}

UINT32 RoomManager::CreateRoom()
{
	//std::lock_guard<std::mutex> guard(mRoomDictLock);
	Room room;
	room.SetRoomNumber(mRoomCount);
	mRoomDict[mRoomCount] = room;
	UINT32 roomNumber = mRoomCount;
	mRoomCount++;
	return roomNumber;
}

void RoomManager::EnterRoom(UINT32 roomNumber, ChatUser* chatUser)
{
	//std::lock_guard<std::mutex> guard(mRoomDictLock);
	mRoomDict[roomNumber].AddUser(chatUser);
}