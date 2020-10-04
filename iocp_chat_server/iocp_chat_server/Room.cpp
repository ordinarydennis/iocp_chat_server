#include "Room.h"
#include "Define.h"
#include "Network.h"
#include "Chatuser.h"

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

void Room::Notify(UINT32 clientFrom, UINT16 packetId, const char* body, size_t bodySize, Network* network)
{
	stPacketHeader header;
	header.mSize = static_cast<UINT16>(bodySize + PACKET_HEADER_SIZE);
	header.mPacket_id = packetId;

	for (ChatUser* user : mUserList)
	{
		network->SendPacket(
			stPacket(
				clientFrom,
				user->GetClientId(),
				header,
				body,
				bodySize
			)
		);
	}
}

