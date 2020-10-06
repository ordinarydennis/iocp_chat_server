#pragma once

#include "Network.h"
#include <basetsd.h>
#include <list>
#include <functional>

class ChatUser;

class Room
{
public:
	UINT32 GetRoomNumber() { return mRoomNumber; };
	std::list<ChatUser*>* GetUserList() { return &mUserList; };
	void SetRoomNumber(UINT32 roomNumber);
	void AddUser(ChatUser* chatUser);
	void RemoveUser(ChatUser* chatUser);
	void Notify(UINT32 clientFrom, UINT16 packetId, const char* body, size_t bodySize, std::function<void(stPacket)> packetSender);

private:
	UINT32						mRoomNumber = 0;
	std::list<ChatUser*>		mUserList;
};

