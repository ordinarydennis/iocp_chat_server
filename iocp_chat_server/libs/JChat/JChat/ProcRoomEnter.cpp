#include "PacketProcessor.h"
#include "ChatUserManager.h"
#include "RoomManager.h"
#include "Logger.h"

namespace JChat
{
	void PacketProcessor::ProcRoomEnter(const JCommon::stPacket& packet)
	{
		ChatUser* chatUser = mChatUserManager->GetUser(packet.mClientFrom);
		if (nullptr == chatUser)
		{
			JCommon::Logger::Error("User Index %d is invalid", packet.mClientFrom);
			JCommon::ResultResPacket resultResPacket;
			resultResPacket.mResult = JCommon::CLIENT_ERROR_CODE::INVALID_USER;
			SendPacket(
				packet.mClientFrom,
				packet.mClientFrom,
				static_cast<UINT16>(JCommon::PACKET_ID::ROOM_ENTER_RES),
				reinterpret_cast<char*>(&resultResPacket),
				sizeof(resultResPacket)
			);
			return;
		}

		JCommon::RoomEnterReqPacket reqPacket(packet.mBody, packet.GetBodySize());

		UINT32 roomNumber = reqPacket.GetRoomNumber();

		JCommon::CLIENT_ERROR_CODE roomError = mRoomManager->EnterRoom(roomNumber, chatUser);

		if (JCommon::CLIENT_ERROR_CODE::INVALID_ROOM_NUMBER == roomError)
		{
			JCommon::Logger::Error("RoomNumber %d is invalid.", roomNumber);
			JCommon::ResultResPacket resultResPacket;
			resultResPacket.mResult = JCommon::CLIENT_ERROR_CODE::INVALID_ROOM_NUMBER;
			SendPacket(
				packet.mClientFrom,
				packet.mClientFrom,
				static_cast<UINT16>(JCommon::PACKET_ID::ROOM_ENTER_RES),
				reinterpret_cast<char*>(&resultResPacket),
				sizeof(resultResPacket)
			);
			return;
		}
		else if (JCommon::CLIENT_ERROR_CODE::FULL_ROOM_USER == roomError)
		{
			JCommon::Logger::Error("Room is full.");
			JCommon::ResultResPacket resultResPacket;
			resultResPacket.mResult = JCommon::CLIENT_ERROR_CODE::FULL_ROOM_USER;
			SendPacket(
				packet.mClientFrom,
				packet.mClientFrom,
				static_cast<UINT16>(JCommon::PACKET_ID::ROOM_ENTER_RES),
				reinterpret_cast<char*>(&resultResPacket),
				sizeof(resultResPacket)
			);
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
		auto userList = room->GetUserList();
		UINT16 userCount = static_cast<UINT16>(userList->size()) - 1; //자신은 제외
		size_t totlaBodySize = 0;
		if (0 < userCount)
		{
			JCommon::RoomUserListNTFPacket roomUserListNTFPacket;
			memcpy_s(roomUserListNTFPacket.mBody, 1, &userCount, 1);
			totlaBodySize++;
			for (const auto& user : *userList)
			{
				UINT64 userUniqueId = user->GetClientId();
				//자기 자신은 리스트에서 제외한다.
				if (userUniqueId == chatUser->GetClientId())
				{
					continue;
				}

				const size_t userUniqueIdSize = sizeof(userUniqueId);
				const size_t bodySize = userUniqueIdSize + JCommon::MAX_USER_ID_BYTE_LENGTH;

				char body[bodySize] = { 0, };
				size_t idLen = user->GetUserId().length();
				memcpy_s(body, userUniqueIdSize, &userUniqueId, userUniqueIdSize);
				memcpy_s(&body[userUniqueIdSize], 1, &idLen, 1);
				memcpy_s(&body[userUniqueIdSize + 1], idLen, user->GetUserId().c_str(), idLen);

				size_t userDataSize = userUniqueIdSize + 1 + idLen;
				memcpy_s(&roomUserListNTFPacket.mBody[totlaBodySize], userDataSize, body, userDataSize);
				totlaBodySize += userDataSize;
			}

			SendPacket(
				packet.mClientFrom,
				packet.mClientFrom,
				static_cast<UINT16>(JCommon::PACKET_ID::ROOM_USER_LIST_NTF),
				reinterpret_cast<char*>(&roomUserListNTFPacket),
				totlaBodySize
			);
		}

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