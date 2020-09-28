#pragma once

#include <memory>
#include "Define.h"

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