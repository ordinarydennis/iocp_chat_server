#pragma once
#include <memory>
#include <string>
#include "RedisTask.h"
#include "Packet.h"
#include "Define.h"

//TODO: 헤더 정리하기!!!!

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
	char mUserId[MAX_USER_ID_BYTE_LENGTH] = { 0, };
	char mUserPw[MAX_USER_PW_BYTE_LENGTH] = { 0, };
};

class LoginResRedisPacket
{
public:
	LoginResRedisPacket(UINT32 clientId, REDIS_TASK_ID redisTaskId, const char* data, size_t data_size)
	{
		mClientId = clientId;
		mRedisTaskId = redisTaskId;
		memcpy_s(mUserId, MAX_USER_ID_BYTE_LENGTH, data, MAX_USER_ID_BYTE_LENGTH);
		memcpy_s(&mResult, sizeof(mResult), &data[MAX_USER_ID_BYTE_LENGTH], sizeof(mResult));
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