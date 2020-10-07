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

//TODO 최흥배
// 방의 개수의 최대 값은 정해져 있으므로 이것도 ClientInfo처럼 객체풀로 사용하는 것이 좋습니다.
// 방 번호와 객체풀에서의 인덱스 위치는 서버의 방 시작 번호만 알명 방 번호로 위치를 쉽게 찾을 수 잇습니다.
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
	//TODO 최흥배
	// map을 이렇게 사용하는 것은 좋지 않습니다. 아마 EnterRoom 함수를 호출전에 객체가 있는 조사했을 것이라고 생각하지만 
	// 만약 조사가 안되었으면 여기서 크래쉬 발생합니다. 방어적 코딩 방법면에서 좋지 않습니다.
	mRoomDict[roomNumber].AddUser(chatUser);
}