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

	//TODO �����. ���� �� �۾��� �ƴϰ� �巡�� �� ������ ���� ���� �� ���Դϴ�.
	// ����ȭ �۾��� �Ѵ�.
	// �½�ũ�� �����͸� ���Ǻ� ���ۿ� �����ϰ�, �ش� �����͸� �����Ѵ�.
#pragma pack(push, 1)

	struct RedisTask
	{
		UINT32 mClientId = 0;
		REDIS_TASK_ID mTaskID = REDIS_TASK_ID::INVALID;
		size_t mDataSize = 0;
		char mData[MAX_REDIS_BUF_SIZE] = { 0, };
	};

	//InterlockedSList �迭�� ����� ��Ŷ ����ü
	struct EntryRedisTask : SLIST_ENTRY
	{
		RedisTask mTask;
	};

#pragma pack(pop)
}

