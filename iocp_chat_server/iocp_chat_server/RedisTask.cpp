#include "RedisTask.h"
#include <memory>

void RedisTask::SetData(const char* data, size_t size)
{
	memcpy_s(mData, size, data, size);
	mDataSize = size;
};