#pragma once

#include "Packet.h"
#include "RoomUser.h"
#include <basetsd.h>
#include <unordered_map>
//#include <list>
#include <functional>

namespace JChat
{
	//class ChatUser;

	class RoomUser;

	class Room
	{
	public:
		Room() = default;

		~Room() = default;

		void					SetRoomNumber(const UINT32 roomNumber);
		
		UINT32					GetRoomNumber() const;

		std::unordered_map<UINT32, RoomUser>* GetUserList();

		void					AddUser(const RoomUser& chatUser);

		void					RemoveUser(UINT32 clientId);

		void					Notify(const UINT32 clientFrom, const UINT16 packetId, const char* body, const size_t bodySize, const std::function<void(JCommon::stPacket)>& packetSender);

	private:
		UINT32						mRoomNumber = 0;
		//std::vector<ChatUser*>		mUserList;
		std::unordered_map<UINT32, RoomUser>	mUserList;
	};
}
