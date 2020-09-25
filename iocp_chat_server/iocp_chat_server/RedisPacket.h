#pragma once
#include <memory>
#include <string>
#include "RedisTask.h"
#include "Define.h"

//TODO: 헤더 정리하기!!!!

class LoginReqRedisPacket
{
public:
	LoginReqRedisPacket(const RedisTask& task)
	{
		mClientId = task.GetClientId();
		mRedisTaskId = task.GetTaskId();
		memcpy_s(mUserPw, USER_PW_BYTE_LENGTH, task.GetData(), task.GetDataSize());
	}
	LoginReqRedisPacket(UINT32 clientId, REDIS_TASK_ID redisTaskId, const char* data, size_t data_size)
	{
		mClientId = clientId;
		mRedisTaskId = redisTaskId;
		memcpy_s(mUserPw, USER_PW_BYTE_LENGTH, data, data_size);
	}
	RedisTask GetTask()
	{
		RedisTask task;
		task.SetClientId(mClientId);
		task.SetTaskId(mRedisTaskId);
		task.SetData(reinterpret_cast<char*>(&mUserPw), strlen(mUserPw));
		return task;
	}

	const UINT32 GetUserId() { return mClientId; };
	const std::string GetUserIdstr() { return std::to_string(mClientId); };
	const char* GetUserPw() { return mUserPw; };

private:
	static const UINT32 USER_PW_BYTE_LENGTH = 33;

	UINT32 mClientId = 0;
	REDIS_TASK_ID mRedisTaskId = REDIS_TASK_ID::INVALID;
	char mUserPw[USER_PW_BYTE_LENGTH] = { 0, };
};

class LoginResRedisPacket
{
public:
	LoginResRedisPacket(UINT32 clientId, REDIS_TASK_ID redisTaskId, const char* data, size_t data_size)
	{
		mClientId = clientId;
		mRedisTaskId = redisTaskId;
		memcpy_s(&mResult, sizeof(mResult), data, data_size);
	}
	RedisTask GetTask()
	{
		RedisTask task;
		task.SetClientId(mClientId);
		task.SetTaskId(mRedisTaskId);
		task.SetData(reinterpret_cast<char*>(&mResult), sizeof(mResult));
		return task;
	}
	ERROR_CODE GetResult()
	{
		return mResult;
	}

private:
	UINT32 mClientId = 0;
	REDIS_TASK_ID mRedisTaskId = REDIS_TASK_ID::INVALID;
	ERROR_CODE mResult = ERROR_CODE::NONE;
};