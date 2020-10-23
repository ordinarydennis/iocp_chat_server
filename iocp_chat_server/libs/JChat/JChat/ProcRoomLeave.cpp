#include "PacketProcessor.h"
#include "ChatUserManager.h"
#include "RoomManager.h"
#include "Logger.h"

namespace JChat
{
	void PacketProcessor::ProcRoomLeave(const JCommon::stPacket& packet)
	{
		ChatUser* chatUser = CheckUserAndSendError(packet.mClientFrom);
		if (nullptr == chatUser)
		{
			return;
		}

		Room* room = mRoomManager->GetRoom(chatUser->GetRoomNumber());
		if (nullptr == room)
		{
			JCommon::Logger::Error("RoomNumber %d is invalid.", chatUser->GetRoomNumber());
			JCommon::ResultResPacket resultResPacket;
			resultResPacket.mResult = JCommon::CLIENT_ERROR_CODE::INVALID_ROOM_NUMBER;
			SendPacket(
				packet.mClientFrom,
				packet.mClientFrom,
				static_cast<UINT16>(JCommon::PACKET_ID::ROOM_LEAVE_RES),
				reinterpret_cast<char*>(&resultResPacket),
				sizeof(resultResPacket)
			);
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

		chatUser->SetState(ChatUser::STATE::LOGIN);

		room->RemoveUser(chatUser->GetClientId());

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