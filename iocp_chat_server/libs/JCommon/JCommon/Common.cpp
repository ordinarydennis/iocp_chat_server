#include "Common.h"
#include <iostream>
#include <chrono>

namespace JCommon
{
	UINT64 GetCurTimeSec()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>
			(std::chrono::system_clock::now().time_since_epoch())
			.count();
	}
}