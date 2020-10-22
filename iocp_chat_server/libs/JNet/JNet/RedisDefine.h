#pragma once

#include <basetsd.h>
#include <memory>

namespace JNet
{
	const size_t MAX_REDIS_BUF_SIZE = 128;

	enum class REDIS_TASK_ID : UINT16
	{
		INVALID = 0,
		REQUEST_LOGIN = 1001,
		RESPONSE_LOGIN = 1002,
	};

	//TODO 최흥배. 지금 할 작업은 아니고 장래에 할 예정을 제가 적어 둔 것입니다.
	// 최적화 작업을 한다.
	// 태스크의 데이터를 세션별 버퍼에 저장하고, 해당 포인터를 전달한다.
#pragma pack(push, 1)

	struct RedisTask
	{
		UINT32 mClientId = 0;
		REDIS_TASK_ID mTaskID = REDIS_TASK_ID::INVALID;
		size_t mDataSize = 0;
		char mData[MAX_REDIS_BUF_SIZE] = { 0, };
	};

	//InterlockedSList 계열에 사용할 패킷 구조체
	struct EntryRedisTask : SLIST_ENTRY
	{
		RedisTask mTask;
	};

#pragma pack(pop)
}

