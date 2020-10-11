#pragma once

#include "Network.h"
#include <basetsd.h>
#include <list>
#include <functional>

class ChatUser;

class Room
{
public:
	UINT32 GetRoomNumber() const { return mRoomNumber; };

	std::vector<ChatUser*>* GetUserList() { return &mUserList; };

	void SetRoomNumber(const UINT32 roomNumber) { mRoomNumber = roomNumber; };

	void AddUser(ChatUser* chatUser) { mUserList.push_back(chatUser); };

	void RemoveUser(const ChatUser* chatUser);

	void Notify(const UINT32 clientFrom, const UINT16 packetId, const char* body, const size_t bodySize, const std::function<void(stPacket)>& packetSender);

private:
	UINT32						mRoomNumber = 0;
	std::vector<ChatUser*>		mUserList;
};

