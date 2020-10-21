#include "PacketProcessor.h"
#include "ChatUserManager.h"
#include "RoomManager.h"
#include "Logger.h"

namespace JChat
{
	void PacketProcessor::ProcRoomChat(const JCommon::stPacket& packet)
	{
		const ChatUser* chatUser = mChatUserManager->GetUser(packet.mClientFrom);
		
		if (nullptr == chatUser)
		{
			JCommon::Logger::Error("User Index %d is invalid", packet.mClientFrom);
			JCommon::ResultResPacket resultResPacket;
			resultResPacket.mResult = JCommon::CLIENT_ERROR_CODE::INVALID_USER;
			SendPacket(
				packet.mClientFrom,
				packet.mClientFrom,
				static_cast<UINT16>(JCommon::PACKET_ID::ROOM_CHAT_RES),
				reinterpret_cast<char*>(&resultResPacket),
				sizeof(resultResPacket)
			);
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
				static_cast<UINT16>(JCommon::PACKET_ID::ROOM_CHAT_RES),
				reinterpret_cast<char*>(&resultResPacket),
				sizeof(resultResPacket)
			);
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