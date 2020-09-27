#include "Room.h"

void Room::SetRoomNumber(UINT32 roomNumber)
{
	mRoomNumber = roomNumber;
}
void Room::AddUser(ChatUser* chatUser)
{
	mUserList.push_back(chatUser);
}
void Room::RemoveUser(ChatUser* chatUser)
{
	mUserList.remove(chatUser);
}

