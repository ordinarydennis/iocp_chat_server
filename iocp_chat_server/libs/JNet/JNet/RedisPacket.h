#pragma once

#include "RedisDefine.h"
#include "Packet.h"

namespace JNet
{
	struct LoginReqRedisPacket
	{
		UINT32 mClientId = 0;
		REDIS_TASK_ID mRedisTaskId = REDIS_TASK_ID::INVALID;
		char mUserId[JCommon::MAX_USER_ID_BYTE_LENGTH + 1] = { 0, };
		char mUserPw[JCommon::MAX_USER_PW_BYTE_LENGTH + 1] = { 0, };

		RedisTask EncodeTask()
		{
			RedisTask task;
			task.SetClientId(mClientId);
			task.SetTaskId(mRedisTaskId);
			char buf[JCommon::MAX_USER_ID_BYTE_LENGTH + JCommon::MAX_USER_PW_BYTE_LENGTH] = { 0, };
			memcpy_s(buf, JCommon::MAX_USER_ID_BYTE_LENGTH, mUserId, JCommon::MAX_USER_ID_BYTE_LENGTH);
			memcpy_s(&buf[JCommon::MAX_USER_ID_BYTE_LENGTH], JCommon::MAX_USER_PW_BYTE_LENGTH, mUserPw, JCommon::MAX_USER_PW_BYTE_LENGTH);
			task.SetData(buf, JCommon::MAX_USER_ID_BYTE_LENGTH + JCommon::MAX_USER_PW_BYTE_LENGTH);
			return task;
		}

		void DecodeTask(const RedisTask& task)
		{
			mClientId = task.GetClientId();
			mRedisTaskId = task.GetTaskId();
			memcpy_s(mUserId, JCommon::MAX_USER_ID_BYTE_LENGTH, task.GetData(), JCommon::MAX_USER_ID_BYTE_LENGTH);
			memcpy_s(mUserPw, JCommon::MAX_USER_PW_BYTE_LENGTH, &task.GetData()[JCommon::MAX_USER_ID_BYTE_LENGTH], JCommon::MAX_USER_PW_BYTE_LENGTH);
		}
	};

	struct LoginResRedisPacket
	{
		UINT32 mClientId = 0;
		REDIS_TASK_ID mRedisTaskId = REDIS_TASK_ID::INVALID;
		char mUserId[JCommon::MAX_USER_ID_BYTE_LENGTH] = { 0, };
		JCommon::CLIENT_ERROR_CODE mResult = JCommon::CLIENT_ERROR_CODE::NONE;

		RedisTask EncodeTask()
		{
			RedisTask task;
			task.SetClientId(mClientId);
			task.SetTaskId(mRedisTaskId);
			char buf[JCommon::MAX_USER_ID_BYTE_LENGTH + sizeof(mResult)] = { 0, };
			memcpy_s(buf, JCommon::MAX_USER_ID_BYTE_LENGTH, mUserId, JCommon::MAX_USER_ID_BYTE_LENGTH);
			memcpy_s(&buf[JCommon::MAX_USER_ID_BYTE_LENGTH], sizeof(mResult), &mResult, sizeof(mResult));
			task.SetData(buf, JCommon::MAX_USER_ID_BYTE_LENGTH + sizeof(mResult));
			return task;
		}

		void DecodeTask(const RedisTask& task)
		{
			mClientId = task.GetClientId();
			mRedisTaskId = task.GetTaskId();
			memcpy_s(mUserId, JCommon::MAX_USER_ID_BYTE_LENGTH, task.GetData(), JCommon::MAX_USER_ID_BYTE_LENGTH);
			memcpy_s(&mResult, sizeof(mResult), &task.GetData()[JCommon::MAX_USER_ID_BYTE_LENGTH], sizeof(mResult));
		}
	};
}
