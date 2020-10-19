#include "PacketProcessor.h"
#include "ChatUserManager.h"
#include "RoomManager.h"

namespace JChat
{
	void PacketProcessor::ProcRoomChat(const JCommon::stPacket& packet)
	{
		const ChatUser* chatUser = mChatUserManager->GetUser(packet.mClientFrom);
		Room* room = mRoomManager->GetRoom(chatUser->GetRoomNumber());
		//TODO 여기 조건문 분리
		if (nullptr == chatUser || nullptr == room)
		{
			return;
		}

		JCommon::RoomChatReqPacket roomChatReqPacket;

		memcpy_s(roomChatReqPacket.mUserId,
			chatUser->GetUserId().length(),
			chatUser->GetUserId().c_str(),
			chatUser->GetUserId().length());

		memcpy_s(roomChatReqPacket.msg,
			strnlen_s(packet.mBody, JCommon::MAX_CHAT_MSG_SIZE),
			packet.mBody,
			strnlen_s(packet.mBody, JCommon::MAX_CHAT_MSG_SIZE));

		room->Notify(
			packet.mClientFrom,
			static_cast<UINT16>(JCommon::PACKET_ID::ROOM_CHAT_NOTIFY),
			reinterpret_cast<char*>(&roomChatReqPacket),
			sizeof(roomChatReqPacket),
			mPacketSender
		);

		JCommon::ResultResPacket resultResPacket;
		resultResPacket.mResult = JCommon::CLIENT_ERROR_CODE::NONE;
		SendPacket(
			packet.mClientFrom,
			packet.mClientFrom,
			static_cast<UINT16>(JCommon::PACKET_ID::ROOM_CHAT_RES),
			reinterpret_cast<char*>(&resultResPacket),
			sizeof(resultResPacket)
		);
	}
}