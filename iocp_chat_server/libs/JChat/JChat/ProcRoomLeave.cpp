#include "PacketProcessor.h"
#include "ChatUserManager.h"
#include "RoomManager.h"

namespace JChat
{
	void PacketProcessor::ProcRoomLeave(const JCommon::stPacket& packet)
	{
		ChatUser* chatUser = mChatUserManager->GetUser(packet.mClientFrom);
		if (nullptr == chatUser)
		{
			//TODO 로그
			return;
		}
		Room* room = mRoomManager->GetRoom(chatUser->GetRoomNumber());
		if (nullptr == room)
		{
			//TODO 로그 
			return;
		}

		JCommon::RoomLeaveNTFPacket roomLeaveNTFPacket;
		roomLeaveNTFPacket.mUniqueId = chatUser->GetClientId();
		room->Notify(
			packet.mClientFrom,
			static_cast<UINT16>(JCommon::PACKET_ID::ROOM_LEAVE_USER_NTF),
			reinterpret_cast<char*>(&roomLeaveNTFPacket),
			sizeof(roomLeaveNTFPacket),
			mPacketSender
		);

		room->RemoveUser(chatUser);

		JCommon::ResultResPacket resultResPacket;
		resultResPacket.mResult = JCommon::CLIENT_ERROR_CODE::NONE;
		SendPacket(
			packet.mClientFrom,
			packet.mClientFrom,
			static_cast<UINT16>(JCommon::PACKET_ID::ROOM_LEAVE_RES),
			reinterpret_cast<char*>(&resultResPacket),
			sizeof(resultResPacket)
		);
	}
}