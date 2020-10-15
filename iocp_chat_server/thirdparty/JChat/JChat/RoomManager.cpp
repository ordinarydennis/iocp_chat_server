#include "RoomManager.h"

namespace JChat
{
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

	//TODO �����
	// ���� ������ �ִ� ���� ������ �����Ƿ� �̰͵� ClientInfoó�� ��üǮ�� ����ϴ� ���� �����ϴ�.
	// �� ��ȣ�� ��üǮ������ �ε��� ��ġ�� ������ �� ���� ��ȣ�� �˸� �� ��ȣ�� ��ġ�� ���� ã�� �� �ս��ϴ�.
	// TODO ���ذ� �� �Ȱ� �����ϱ�
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
}