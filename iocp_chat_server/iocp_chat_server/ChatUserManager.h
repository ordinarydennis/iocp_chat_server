#pragma once

#include "ChatUser.h"
#include <basetsd.h>
#include <unordered_map>

class ChatUserManager
{
public:
	ChatUserManager();
	~ChatUserManager();

	ChatUser* GetUser(const UINT32 userId);

	void AddUser(const ChatUser& chatUser);


private:
	std::unordered_map<UINT32, ChatUser>	mChatUserDict;
};

