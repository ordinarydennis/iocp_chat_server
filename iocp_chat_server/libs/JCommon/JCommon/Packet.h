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

	struct stPacketHeader
	{
		UINT16			mSize = 0;
		UINT16			mPacket_id = 0;
	};

	const UINT16 PACKET_HEADER_SIZE = sizeof(stPacketHeader) + 1;

	struct stPacket
	{
		UINT32			mClientFrom = 0;
		UINT32			mClientTo = 0;		//TODO 제거하기
		stPacketHeader	mHeader;
		char			mBody[MAX_SOCKBUF] = { 0, };

		stPacket()
		{
			ZeroMemory(&mBody, MAX_SOCKBUF);
		};

		stPacket(UINT32 ClientFrom, UINT32 ClientTo, stPacketHeader Header, const char* Body, size_t size)
		{
			mClientFrom = ClientFrom;
			mClientTo = ClientTo;
			mHeader = Header;
			memcpy_s(mBody, size, Body, size);
		}

		size_t GetBodySize() const
		{
			return mHeader.mSize - PACKET_HEADER_SIZE;
		}
	};

	struct EntryPacket
	{
		SLIST_ENTRY			mEntry;
		JCommon::stPacket	mPacket;
	};

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