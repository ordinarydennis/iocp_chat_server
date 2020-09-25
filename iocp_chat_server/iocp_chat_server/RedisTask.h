#pragma once

#include "RedisDefine.h"

class RedisTask
{
public:
	RedisTask() = default;
	~RedisTask() = default;

	UINT32 GetClientId() const { return mClientId; };
	REDIS_TASK_ID GetTaskId() const { return mTaskID; };
	const char* GetData() const { return mData; };
	size_t GetDataSize() const { return mDataSize; };

	void SetClientId(UINT32 clientId);
	void SetTaskId(REDIS_TASK_ID id);
	void SetData(const char* data, size_t size);

private:
	UINT32 mClientId = 0;
	REDIS_TASK_ID mTaskID = REDIS_TASK_ID::INVALID;
	size_t mDataSize = 0;

	char mData[MAX_REDIS_BUF_SIZE] = { 0, };
};

