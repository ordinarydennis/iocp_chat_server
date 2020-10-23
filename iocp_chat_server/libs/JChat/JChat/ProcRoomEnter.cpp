#include "PacketProcessor.h"
#include "ChatUserManager.h"
#include "RoomManager.h"
#include "Logger.h"
#include "RoomUser.h"

namespace JChat
{
	void PacketProcessor::ProcRoomEnter(const JCommon::stPacket& packet)
	{
		ChatUser* chatUser = CheckUserAndSendError(packet.mClientFrom);
		if (nullptr == chatUser)
		{
			return;
		}

		JCommon::RoomEnterReqPacket reqPacket(packet.mBody, packet.GetBodySize());
		
		UINT32 roomNumber = reqPacket.GetRoomNumber();
		RoomUser roomUser;
		roomUser.SetClientId(chatUser->GetClientId());
		roomUser.SetUserId(chatUser->GetUserId());
		if (false == EnterRoomAndSendError(roomNumber, roomUser))
		{
			return;
		}

		chatUser->SetRoomNumber(roomNumber);
		chatUser->SetState(ChatUser::STATE::ENTER_ROOM);

		JCommon::ResultResPacket resultResPacket;
		resultResPacket.mResult = JCommon::CLIENT_ERROR_CODE::NONE;
		SendPacket(
			packet.mClientFrom,
			packet.mClientFrom,
			static_cast<UINT16>(JCommon::PACKET_ID::ROOM_ENTER_RES),
			reinterpret_cast<char*>(&resultResPacket),
			sizeof(resultResPacket)
		);

		//방 유저 리스트 내려주기
		Room* room = mRoomManager->GetRoom(roomNumber);
		SendRoomUserList(room, chatUser->GetClientId());

		//방 유저들에게 노티
		JCommon::RoomEnterNTFPacket roomEnterNTFPacket;
		roomEnterNTFPacket.mUniqueId = chatUser->GetClientId();
		roomEnterNTFPacket.uidLen = static_cast<char>(chatUser->GetUserId().length());
		memcpy_s(
			roomEnterNTFPacket.mUserId,
			roomEnterNTFPacket.uidLen,
			chatUser->GetUserId().c_str(),
			chatUser->GetUserId().length()
		);

		room->Notify(
			chatUser->GetClientId(),
			static_cast<UINT16>(JCommon::PACKET_ID::ROOM_NEW_USER_NTF),
			reinterpret_cast<char*>(&roomEnterNTFPacket),
			sizeof(roomEnterNTFPacket),
			mPacketSender
		);
	}
}