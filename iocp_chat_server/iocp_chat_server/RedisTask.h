#pragma once

#include "RedisDefine.h"

//TODO 최흥배. 지금 할 작업은 아니고 장래에 할 예정을 제가 적어 둔 것입니다.
// 최적화 작업을 한다.
// 태스크의 데이터를 세션별 버퍼에 저장하고, 해당 포인터를 전달한다.

class RedisTask
{
public:
	RedisTask() = default;
	~RedisTask() = default;

	UINT32 GetClientId() const { return mClientId; };
	RedisTaskID GetTaskId() const { return mTaskID; };
	const char* GetData() const { return mData; };
	size_t GetDataSize() const { return mDataSize; };

	void SetClientId(const UINT32 clientId) { mClientId = clientId; };
	void SetTaskId(const RedisTaskID id) { mTaskID = id; };
	void SetData(const char* data, const size_t size);

private:
	UINT32 mClientId = 0;
	RedisTaskID mTaskID = RedisTaskID::INVALID;
	size_t mDataSize = 0;

	char mData[MAX_REDIS_BUF_SIZE] = { 0, };
};

