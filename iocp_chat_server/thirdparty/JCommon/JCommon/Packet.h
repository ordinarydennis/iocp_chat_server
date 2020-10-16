#pragma once

#include "Common.h"
#include <memory>
#include <vector>
#include <string>

namespace JCommon
{
	const int MAX_USER_ID_BYTE_LENGTH = 33;
	const int MAX_USER_PW_BYTE_LENGTH = 33;
	const int MAX_CHAT_MSG_SIZE = 257;

#pragma pack(push, 1)

	struct LoginReqPacket
	{
		char mUserId[MAX_USER_ID_BYTE_LENGTH] = { 0, };
		char mUserPw[MAX_USER_PW_BYTE_LENGTH] = { 0, };
	};

	struct ResultResPacket
	{
		CLIENT_ERROR_CODE mResult;
	};

	class RoomEnterReqPacket
	{
	public:
		RoomEnterReqPacket(const char* buf, size_t size)
		{
			memcpy_s(&mRoomNumber, sizeof(mRoomNumber), buf, size);
		}
		UINT32 GetRoomNumber() { return mRoomNumber; };
	private:
		UINT32 mRoomNumber = 0;
	};

	struct RoomUserListNTFPacket
	{
		char mBody[MAX_SOCKBUF] = { 0, };
	};

	struct RoomEnterNTFPacket
	{
		UINT64 mUniqueId = 0;
		char uidLen = 0;
		char mUserId[MAX_USER_ID_BYTE_LENGTH] = { 0, };

	};

	struct RoomChatReqPacket
	{
		char mUserId[MAX_USER_ID_BYTE_LENGTH] = { 0, };
		char msg[MAX_USER_ID_BYTE_LENGTH + MAX_CHAT_MSG_SIZE] = { 0, };
	};

	struct RoomLeaveNTFPacket
	{
		UINT64 mUniqueId = 0;
	};

#pragma pack(pop)
}