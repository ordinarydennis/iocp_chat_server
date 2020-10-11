#include "RedisTask.h"
#include <memory>

void RedisTask::SetData(const char* data, const size_t size)
{
	memcpy_s(mData, size, data, size);
	mDataSize = size;
};