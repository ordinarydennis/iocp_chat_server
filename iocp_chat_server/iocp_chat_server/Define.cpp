#include "Define.h"
#include <chrono>

UINT64 GetCurTimeSec()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>
		(std::chrono::system_clock::now().time_since_epoch())
		.count();
}