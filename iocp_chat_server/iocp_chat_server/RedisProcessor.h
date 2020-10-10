#pragma once

#include "RedisPacket.h"

#include <unordered_map>

class RedisProcessor
{
public:
	RedisProcessor();

	~RedisProcessor();

	void ProcessRedisPacket(const RedisTask& task);

private:
	void ProcLogin(const RedisTask& task);

private:
	using receiver = void(RedisProcessor::*)(const RedisTask& p);
	std::unordered_map<RedisTaskID, receiver>  mRecvProcDict;
};

