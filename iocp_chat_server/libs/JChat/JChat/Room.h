#pragma once

#include "Packet.h"
#include <basetsd.h>
#include <list>
#include <functional>

namespace JChat
{
	class ChatUser;

	class Room
	{
	public:
		Room() = default;

		~Room() = default;

		void SetRoomNumber(const UINT32 roomNumber);
		
		UINT32 GetRoomNumber() const;

		std::vector<ChatUser*>* GetUserList();

		void AddUser(ChatUser* chatUser);

		void RemoveUser(const ChatUser* chatUser);

		void Notify(const UINT32 clientFrom, const UINT16 packetId, const char* body, const size_t bodySize, const std::function<void(JCommon::stPacket)>& packetSender);

	private:
		UINT32						mRoomNumber = 0;
		std::vector<ChatUser*>		mUserList;
	};
}
