#pragma once

#include "Define.h"
#include "ChatUser.h"
#include <memory>
#include <list>

const int MAX_USER_ID_BYTE_LENGTH = 33;
const int MAX_USER_PW_BYTE_LENGTH = 33;
const int MAX_CHAT_MSG_SIZE = 257;

class LoginReqPacket
{
public:
	void SetPacket(const char* buf)
	{
		memcpy_s(mUserId, MAX_USER_ID_BYTE_LENGTH, buf, MAX_USER_ID_BYTE_LENGTH);
		memcpy_s(mUserPw, MAX_USER_PW_BYTE_LENGTH, buf + MAX_USER_ID_BYTE_LENGTH, MAX_USER_PW_BYTE_LENGTH);
	}
	const char* GetUserId() { return mUserId; };
	const char* GetUserPw() { return mUserPw; };

private:
	char mUserId[MAX_USER_ID_BYTE_LENGTH] = { 0, };
	char mUserPw[MAX_USER_PW_BYTE_LENGTH] = { 0, };
};

//class LoginResPacket
//{
//public:
//	void SetResult(ERROR_CODE result) { mResult = result; };
//	ERROR_CODE GetResult() { return mResult; };
//
//private:
//	ERROR_CODE mResult;
//};

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

//class RoomEnterResPacket
//{
//public:
//	void SetResult(ERROR_CODE result) { mResult = result; };
//	ERROR_CODE GetResult() { return mResult; };
//
//private:
//	ERROR_CODE mResult;
//};

class RoomUserListNTFPacket
{
public:
	RoomUserListNTFPacket(UINT32 enteredClientId, std::list<ChatUser*>& userList)
	{
		UINT16 userCount = static_cast<UINT16>(userList.size()) - 1; //자신은 제외
		memcpy_s(mBody, 1, &userCount, 1);
		mBodySize++;
		for (auto user : userList)
		{
			UINT64 userUniqueId = user->GetClientId();
			//자기 자신은 리스트에서 제외한다.
			if (userUniqueId == enteredClientId)
				continue;

			const size_t userUniqueIdSize = sizeof(userUniqueId);
			const size_t bodySize = userUniqueIdSize + MAX_USER_ID_BYTE_LENGTH;

			char body[bodySize] = { 0, };
			size_t idLen = user->GetUserId().length();
			memcpy_s(body, userUniqueIdSize, &userUniqueId, userUniqueIdSize);
			memcpy_s(&body[userUniqueIdSize], 1, &idLen, 1);
			memcpy_s(&body[userUniqueIdSize + 1], user->GetUserId().length(), user->GetUserId().c_str(), user->GetUserId().length());

			size_t userDataSize = userUniqueIdSize + 1 + user->GetUserId().length();
			memcpy_s(&mBody[mBodySize], userDataSize, body, userDataSize);
			mBodySize += userDataSize;
		}
	}
	char* GetBody()
	{
		return mBody;
	}
	size_t GetBodySize()
	{
		return mBodySize;
	}

private:
	char mBody[MAX_SOCKBUF] = { 0, };
	size_t mBodySize = 0;
};

class RoomEnterNTFPacket
{
public:
	RoomEnterNTFPacket(UINT64 uniqueId, std::string userId)
	{
		mUniqueId = uniqueId;
		mUserId = userId;
	}
	char* GetBody()
	{
		memcpy_s(mBody, UNIQUE_ID_SIZE, &mUniqueId, UNIQUE_ID_SIZE);
		size_t idLen = mUserId.length();
		memcpy_s(&mBody[UNIQUE_ID_SIZE], 1, &idLen, 1);
		memcpy_s(&mBody[UNIQUE_ID_SIZE + 1], idLen, mUserId.c_str(), idLen);
		return mBody;
	}
	size_t GetBodySize()
	{
		return BODY_SIZE;
	}

private:
	UINT64 mUniqueId = 0;
	std::string mUserId;

	static const size_t UNIQUE_ID_SIZE = sizeof(mUniqueId);
	static const size_t BODY_SIZE = sizeof(mUniqueId) + MAX_USER_ID_BYTE_LENGTH;
	
	char mBody[BODY_SIZE] = { 0, };
};

//class RoomChatReqPacket
//{
//public:
//	RoomChatReqPacket(const char* buf)
//	{
//		memcpy_s(&chat, strlen(buf), buf, strlen(buf));
//	}
//	const char* GetChat() const { return chat; };
//private:
//	char chat[MAX_CHAT_MSG_SIZE] = { 0, };
//};

//class RoomChatResPacket
//{
//public:
//	void SetResult(ERROR_CODE result) { mResult = result; };
//	ERROR_CODE GetResult() { return mResult; };
//
//private:
//	ERROR_CODE mResult;
//};