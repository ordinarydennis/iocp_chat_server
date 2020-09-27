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
	//TODO : Lock
	Room room;
	room.SetRoomNumber(mRoomCount);
	mRoomDict[mRoomCount] = room;
	UINT32 roomNumber = mRoomCount;
	mRoomCount++;
	return roomNumber;
}
void RoomManager::EnterRoom(UINT32 roomNumber, ChatUser* chatUser)
{
	//TODO : Lock
	//존재하는 방인지 확인

	mRoomDict[roomNumber].AddUser(chatUser);
}