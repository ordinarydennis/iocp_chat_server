#include "Room.h"
#include "Define.h"
#include "Network.h"
#include "Chatuser.h"

void Room::Notify(UINT32 clientFrom, UINT16 packetId, const char* body, size_t bodySize, const std::function<void(stPacket)>& packetSender)
{
	stPacketHeader header;
	header.mSize = static_cast<UINT16>(bodySize + PACKET_HEADER_SIZE);
	header.mPacket_id = packetId;

	for (ChatUser* user : mUserList)
	{
		packetSender(
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

