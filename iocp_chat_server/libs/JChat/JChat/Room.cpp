#include "Room.h"
#include "Network.h"

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

	std::unordered_map<UINT32, RoomUser>* Room::GetUserList()
	{ 
		return &mUserList; 
	};

	void Room::AddUser(const RoomUser& roomUser)
	{ 
		mUserList[roomUser.GetClientId()] = roomUser;
	}

	void Room::RemoveUser(UINT32 clientId)
	{
		mUserList.erase(clientId);
	}

	void Room::Notify(const UINT32 clientFrom, const UINT16 packetId, const char* body, const size_t bodySize, const std::function<void(JCommon::stPacket)>& packetSender)
	{
		JCommon::stPacketHeader header;
		header.mSize = static_cast<UINT16>(bodySize + JCommon::PACKET_HEADER_SIZE);
		header.mPacket_id = packetId;

		for (auto& roomUserPair : mUserList)
		{
			packetSender(
				JCommon::stPacket(
					clientFrom,
					roomUserPair.second.GetClientId(),
					header,
					body,
					bodySize
				)
			);
		}
	}
}