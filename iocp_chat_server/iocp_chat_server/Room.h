#pragma once

#include <basetsd.h>
#include <list>

class ChatUser;

class Room
{
public:
	UINT32 GetRoomNumber() { return mRoomNumber; };
	std::list<ChatUser*>* GetUserList() { return &mUserList; };
	void SetRoomNumber(UINT32 roomNumber);
	void AddUser(ChatUser* chatUser);

private:
	UINT32					mRoomNumber = 0;
	std::list<ChatUser*>		mUserList;
};

