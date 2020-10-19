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

	class RedisTask
	{
	public:
		RedisTask() = default;
		~RedisTask() = default;

		UINT32 GetClientId() const { return mClientId; };
		REDIS_TASK_ID GetTaskId() const { return mTaskID; };
		const char* GetData() const { return mData; };
		size_t GetDataSize() const { return mDataSize; };

		void SetClientId(const UINT32 clientId) { mClientId = clientId; };
		void SetTaskId(const REDIS_TASK_ID id) { mTaskID = id; };
		void SetData(const char* data, const size_t size)
		{
			memcpy_s(mData, size, data, size);
			mDataSize = size;
		}

	private:
		UINT32 mClientId = 0;
		REDIS_TASK_ID mTaskID = REDIS_TASK_ID::INVALID;
		size_t mDataSize = 0;

		char mData[MAX_REDIS_BUF_SIZE] = { 0, };
	};
}

