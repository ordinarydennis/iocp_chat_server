#include "Room.h"
#include "Network.h"
#include "Chatuser.h"

namespace JChat
{
	void Room::SetRoomNumber(UINT32 roomNumber)
	{
		mRoomNumber = roomNumber;
	}

	UINT32 Room::GetRoomNumber() const
	{
		return mRoomNumber;
	}

	std::vector<ChatUser*>* Room::GetUserList() 
	{ 
		return &mUserList; 
	};

	void Room::AddUser(ChatUser* chatUser)
	{ 
		mUserList.push_back(chatUser); 
	}

	void Room::RemoveUser(const ChatUser* chatUser)
	{
		mUserList.erase(std::remove(mUserList.begin(), mUserList.end(), chatUser), mUserList.end());
	}

	void Room::Notify(const UINT32 clientFrom, const UINT16 packetId, const char* body, const size_t bodySize, const std::function<void(JCommon::stPacket)>& packetSender)
	{
		JCommon::stPacketHeader header;
		header.mSize = static_cast<UINT16>(bodySize + JCommon::PACKET_HEADER_SIZE);
		header.mPacket_id = packetId;

		for (ChatUser* user : mUserList)
		{
			packetSender(
				JCommon::stPacket(
					clientFrom,
					user->GetClientId(),
					header,
					body,
					bodySize
				)
			);
		}
	}
}