#include "PacketProcessor.h"
#include "Packet.h"
#include "Network.h"
#include "ClientInfo.h"
#include "ChatUserManager.h"
#include "RoomManager.h"

namespace JChat
{
	void PacketProcessor::ProcCloseSocket(const JCommon::stPacket& packet)
	{
		const JCommon::CloseSocketReqPacket* closeSocketReqPacket = reinterpret_cast<const JCommon::CloseSocketReqPacket*>(&packet.mBody);

		JCommon::Logger::Info("socket(%d) 접속 끊김", (int)closeSocketReqPacket->mUniqueId);
		mNetwork->CloseSocket(closeSocketReqPacket->mUniqueId);

		auto chatUser = mChatUserManager->GetUser(closeSocketReqPacket->mUniqueId);
		
		//접속만 하고 로그인은 안한 유저
		if (nullptr == chatUser)
		{
			return;
		}

		auto state = chatUser->GetState();
		if (ChatUser::STATE::ENTER_ROOM == state)
		{
			Room* room = mRoomManager->GetRoom(chatUser->GetRoomNumber());
			if (nullptr == room)
			{
				JCommon::Logger::Error("RoomNumber %d is invalid.", chatUser->GetRoomNumber());
				return;
			}

			room->RemoveUser(chatUser->GetClientId());

			JCommon::RoomLeaveNTFPacket roomLeaveNTFPacket;
			roomLeaveNTFPacket.mUniqueId = chatUser->GetClientId();
			room->Notify(
				packet.mClientFrom,
				static_cast<UINT16>(JCommon::PACKET_ID::ROOM_LEAVE_USER_NTF),
				reinterpret_cast<char*>(&roomLeaveNTFPacket),
				sizeof(roomLeaveNTFPacket),
				mPacketSender
			);
		}
		mChatUserManager->RemoveUser(chatUser->GetClientId());
	}
}
