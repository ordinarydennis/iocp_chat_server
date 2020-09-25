#pragma once

#include <basetsd.h>

const size_t MAX_REDIS_BUF_SIZE = 128;

enum class REDIS_TASK_ID : UINT16
{
	INVALID = 0,
	REQUEST_LOGIN = 1001,
	RESPONSE_LOGIN = 1002,
};
