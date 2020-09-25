#pragma once

#include <memory>

class LoginReqPacket
{
public:
	static const int MAX_USER_ID_BYTE_LENGTH = 33;
	static const int MAX_USER_PW_BYTE_LENGTH = 33;

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

class LoginResPacket
{
public:
	void SetResult(ERROR_CODE result) { mResult = result; };
	ERROR_CODE GetResult() { return mResult;};

private:
	ERROR_CODE mResult;
};