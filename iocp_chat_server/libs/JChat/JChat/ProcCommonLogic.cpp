#include "PacketProcessor.h"
#include "Logger.h"
#include "ChatUserManager.h"
#include "Packet.h"
#include "RoomManager.h"

namespace JChat
{
	ChatUser* PacketProcessor::CheckUserAndSendError(UINT32 clientId)
	{
		ChatUser* chatUser = mChatUserManager->GetUser(clientId);
		if (nullptr == chatUser)
		{
			JCommon::Logger::Error("No Chatting User Index %d is invalid", clientId);
			JCommon::ResultResPacket resultResPacket;
			resultResPacket.mResult = JCommon::CLIENT_ERROR_CODE::INVALID_USER;
			SendPacket(
				clientId,
				clientId,
				static_cast<UINT16>(JCommon::PACKET_ID::ROOM_ENTER_RES),
				reinterpret_cast<char*>(&resultResPacket),
				sizeof(resultResPacket)
			);
		}
		return chatUser;
	}

	bool PacketProcessor::EnterRoomAndSendError(UINT32 roomNumber, const RoomUser& roomUser)
	{
		JCommon::CLIENT_ERROR_CODE roomError = mRoomManager->EnterRoom(roomNumber, roomUser);

		if (JCommon::CLIENT_ERROR_CODE::INVALID_ROOM_NUMBER == roomError)
		{
			JCommon::Logger::Error("RoomNumber %d is invalid.", roomNumber);
			JCommon::ResultResPacket resultResPacket;
			resultResPacket.mResult = JCommon::CLIENT_ERROR_CODE::INVALID_ROOM_NUMBER;
			SendPacket(
				roomUser.GetClientId(),
				roomUser.GetClientId(),
				static_cast<UINT16>(JCommon::PACKET_ID::ROOM_ENTER_RES),
				reinterpret_cast<char*>(&resultResPacket),
				sizeof(resultResPacket)
			);
			return false;
		}
		else if (JCommon::CLIENT_ERROR_CODE::FULL_ROOM_USER == roomError)
		{
			JCommon::Logger::Error("Room is full.");
			JCommon::ResultResPacket resultResPacket;
			resultResPacket.mResult = JCommon::CLIENT_ERROR_CODE::FULL_ROOM_USER;
			SendPacket(
				roomUser.GetClientId(),
				roomUser.GetClientId(),
				static_cast<UINT16>(JCommon::PACKET_ID::ROOM_ENTER_RES),
				reinterpret_cast<char*>(&resultResPacket),
				sizeof(resultResPacket)
			);
			return false;
		}
		return true;
	}

	void PacketProcessor::SendRoomUserList(Room* room, const UINT32 recvClientId)
	{
		auto roomUserList = room->GetUserList();
		UINT16 userCount = static_cast<UINT16>(roomUserList->size()) - 1; //자신은 제외
		size_t totlaBodySize = 0;
		if (0 < userCount)
		{
			JCommon::RoomUserListNTFPacket roomUserListNTFPacket;
			memcpy_s(roomUserListNTFPacket.mBody, 1, &userCount, 1);
			totlaBodySize++;
			for (const auto& roomUserPair : *roomUserList)
			{
				auto roomUser = roomUserPair.second;
				UINT64 userUniqueId = roomUser.GetClientId();
				//자기 자신은 리스트에서 제외한다.
				if (userUniqueId == recvClientId)
				{
					continue;
				}

				const size_t userUniqueIdSize = sizeof(userUniqueId);
				const size_t bodySize = userUniqueIdSize + JCommon::MAX_USER_ID_BYTE_LENGTH;

				char body[bodySize] = { 0, };
				size_t idLen = roomUser.GetUserId().length();
				memcpy_s(body, userUniqueIdSize, &userUniqueId, userUniqueIdSize);
				memcpy_s(&body[userUniqueIdSize], 1, &idLen, 1);
				memcpy_s(&body[userUniqueIdSize + 1], idLen, roomUser.GetUserId().c_str(), idLen);

				size_t userDataSize = userUniqueIdSize + 1 + idLen;
				memcpy_s(&roomUserListNTFPacket.mBody[totlaBodySize], userDataSize, body, userDataSize);
				totlaBodySize += userDataSize;
			}

			SendPacket(
				recvClientId,
				recvClientId,
				static_cast<UINT16>(JCommon::PACKET_ID::ROOM_USER_LIST_NTF),
				reinterpret_cast<char*>(&roomUserListNTFPacket),
				totlaBodySize
			);
		}
	}

}
