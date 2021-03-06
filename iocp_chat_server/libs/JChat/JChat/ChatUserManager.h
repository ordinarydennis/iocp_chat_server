#pragma once

#include "ChatUser.h"
#include <basetsd.h>
#include <unordered_map>
#include <mutex>

namespace JChat
{
	class ChatUserManager
	{
	public:
		ChatUserManager() = default;

		~ChatUserManager() = default;

		ChatUser* GetUser(const UINT32 userId);

		void AddUser(const ChatUser& chatUser);

		void RemoveUser(const UINT32 userId);

	private:
		std::unordered_map<UINT32, ChatUser>	mChatUserDict;
	};
}


