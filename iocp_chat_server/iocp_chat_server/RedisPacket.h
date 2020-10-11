#pragma once
#include <memory>
#include <string>
#include "RedisTask.h"
#include "Packet.h"
#include "Define.h"

struct LoginReqRedisPacket
{
	UINT32 mClientId = 0;
	RedisTaskID mRedisTaskId = RedisTaskID::INVALID;
	char mUserId[MAX_USER_ID_BYTE_LENGTH + 1] = { 0, };
	char mUserPw[MAX_USER_PW_BYTE_LENGTH + 1] = { 0, };

	RedisTask EncodeTask()
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

	void DecodeTask(const RedisTask& task)
	{
		mClientId = task.GetClientId();
		mRedisTaskId = task.GetTaskId();
		memcpy_s(mUserId, MAX_USER_ID_BYTE_LENGTH, task.GetData(), MAX_USER_ID_BYTE_LENGTH);
		memcpy_s(mUserPw, MAX_USER_PW_BYTE_LENGTH, &task.GetData()[MAX_USER_ID_BYTE_LENGTH], MAX_USER_PW_BYTE_LENGTH);
	}
};

struct LoginResRedisPacket
{
	UINT32 mClientId = 0;
	RedisTaskID mRedisTaskId = RedisTaskID::INVALID;
	char mUserId[MAX_USER_ID_BYTE_LENGTH + 1] = { 0, };
	ERROR_CODE mResult = ERROR_CODE::NONE;

	RedisTask EncodeTask()
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

	void DecodeTask(const RedisTask& task)
	{
		mClientId = task.GetClientId();
		mRedisTaskId = task.GetTaskId();
		memcpy_s(mUserId, MAX_USER_ID_BYTE_LENGTH, task.GetData(), MAX_USER_ID_BYTE_LENGTH);
		memcpy_s(&mResult, sizeof(mResult), &task.GetData()[MAX_USER_ID_BYTE_LENGTH], sizeof(mResult));
	}
};