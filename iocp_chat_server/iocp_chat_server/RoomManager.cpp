#include "RoomManager.h"

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

//TODO 최흥배
// 방의 개수의 최대 값은 정해져 있으므로 이것도 ClientInfo처럼 객체풀로 사용하는 것이 좋습니다.
// 방 번호와 객체풀에서의 인덱스 위치는 서버의 방 시작 번호만 알명 방 번호로 위치를 쉽게 찾을 수 잇습니다.
// TODO 이해가 잘 안감 질문하기
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
	if (false == IsExistRoom(roomNumber))
	{
		roomNumber = CreateRoom();
	}
	mRoomDict[roomNumber].AddUser(chatUser);
}