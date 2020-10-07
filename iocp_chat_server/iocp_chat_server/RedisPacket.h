#pragma once
#include <memory>
#include <string>
#include "RedisTask.h"
#include "Packet.h"
#include "Define.h"

//TODO 최흥배
//이 방법 조금 위험한 것 같습니다. 보통 자주 사용하는 방법도 아니고요
//이 클래스의 외부에서 멤버변수에 값을 채우고, 이 클래스의 encoding(함수 이름은 원하는대로) 함수를 호출하면 버퍼에 데이터를 쓰고, decoding을 하면 버퍼에서 멤버 변수에 값을 넣도록 하는 것이 일반적입니다.
//그리고 이것으 클래스가 어떤 동작을 가지고 있는 것보다 데이터를 가지고 있는 것이 목적입니다. class 보다 struct가 더 좋습니다.
class LoginReqRedisPacket
{
public:
	LoginReqRedisPacket(const RedisTask& task)
	{
		mClientId = task.GetClientId();
		mRedisTaskId = task.GetTaskId();
		memcpy_s(mUserId, MAX_USER_ID_BYTE_LENGTH, task.GetData(), MAX_USER_ID_BYTE_LENGTH);
		memcpy_s(mUserPw, MAX_USER_PW_BYTE_LENGTH, &task.GetData()[MAX_USER_ID_BYTE_LENGTH], MAX_USER_PW_BYTE_LENGTH);
	}
	LoginReqRedisPacket(UINT32 clientId, REDIS_TASK_ID redisTaskId, const char* data, size_t data_size)
	{
		mClientId = clientId;
		mRedisTaskId = redisTaskId;
		memcpy_s(mUserId, MAX_USER_ID_BYTE_LENGTH, data, MAX_USER_ID_BYTE_LENGTH);
		memcpy_s(mUserPw, MAX_USER_PW_BYTE_LENGTH, &data[MAX_USER_ID_BYTE_LENGTH], MAX_USER_PW_BYTE_LENGTH);
	}
	RedisTask GetTask()
	{
		RedisTask task;
		task.SetClientId(mClientId);
		task.SetTaskId(mRedisTaskId);

		char buf[MAX_USER_ID_BYTE_LENGTH + MAX_USER_PW_BYTE_LENGTH] = { 0, };
		memcpy_s(buf, MAX_USER_ID_BYTE_LENGTH, mUserId, MAX_USER_ID_BYTE_LENGTH);
		memcpy_s(&buf[MAX_USER_ID_BYTE_LENGTH], MAX_USER_PW_BYTE_LENGTH, mUserPw, MAX_USER_PW_BYTE_LENGTH);

		task.SetData(buf, MAX_USER_ID_BYTE_LENGTH + MAX_USER_PW_BYTE_LENGTH);
		return task;
	}

	const UINT32 GetClientId() { return mClientId; };
	const std::string GetClientIdstr() { return std::to_string(mClientId); };
	const char* GetUserId() { return mUserId; };
	const char* GetUserPw() { return mUserPw; };

private:

	UINT32 mClientId = 0;
	REDIS_TASK_ID mRedisTaskId = REDIS_TASK_ID::INVALID;

	//TODO 최흥배
	// 아래 변수를 그대로 문자열 비교에 사용한다면 널문자가 들어갈 공간도 필요합니다.
	// 최대 숫자에 1을 더 해야 합니다.
	char mUserId[MAX_USER_ID_BYTE_LENGTH] = { 0, };
	char mUserPw[MAX_USER_PW_BYTE_LENGTH] = { 0, };
};

class LoginResRedisPacket
{
public:
	LoginResRedisPacket(UINT32 clientId, REDIS_TASK_ID redisTaskId, const char* data)
	{
		mClientId = clientId;
		mRedisTaskId = redisTaskId;
		memcpy_s(mUserId, MAX_USER_ID_BYTE_LENGTH, data, MAX_USER_ID_BYTE_LENGTH);
		memcpy_s(&mResult, sizeof(mResult), &data[MAX_USER_ID_BYTE_LENGTH], sizeof(mResult));
	}
	LoginResRedisPacket(const RedisTask& task)
	{
		mClientId = task.GetClientId();
		mRedisTaskId = task.GetTaskId();
		memcpy_s(mUserId, MAX_USER_ID_BYTE_LENGTH, task.GetData(), MAX_USER_ID_BYTE_LENGTH);
		memcpy_s(&mResult, sizeof(mResult), &task.GetData()[MAX_USER_ID_BYTE_LENGTH], sizeof(mResult));
	}
	RedisTask GetTask()
	{
		RedisTask task;
		task.SetClientId(mClientId);
		task.SetTaskId(mRedisTaskId);
		char buf[MAX_USER_ID_BYTE_LENGTH + 2] = { 0, };
		memcpy_s(buf, MAX_USER_ID_BYTE_LENGTH, mUserId, MAX_USER_ID_BYTE_LENGTH);
		memcpy_s(&buf[MAX_USER_ID_BYTE_LENGTH], MAX_USER_PW_BYTE_LENGTH, &mResult, sizeof(mResult));
		task.SetData(buf, MAX_USER_ID_BYTE_LENGTH + 2);
		return task;
	}
	std::string GetUserId()
	{
		return std::string(mUserId);
	}
	ERROR_CODE GetResult()
	{
		return mResult;
	}

private:
	UINT32 mClientId = 0;
	REDIS_TASK_ID mRedisTaskId = REDIS_TASK_ID::INVALID;
	char mUserId[MAX_USER_ID_BYTE_LENGTH] = { 0, };
	ERROR_CODE mResult = ERROR_CODE::NONE;
};