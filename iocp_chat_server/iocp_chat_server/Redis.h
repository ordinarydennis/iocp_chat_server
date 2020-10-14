#pragma once

#include "RedisTask.h"
#include "Define.h"
#include <vector>
#include <thread>
#include <queue>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace RedisCpp
{
	class CRedisConn;
}

class Redis
{
public:
	Redis();

	~Redis();

	Error Connect(const char* ip, const UINT16 port);

	void Run();

	void Destroy();

	void RequestTask(const RedisTask& task);

	std::optional<RedisTask> GetResponseTask();

	std::optional<RedisTask> GetRequestTask();

private:
	void ResponseTask(const RedisTask& task);

	void RedisThread();

	void ProcessRedisPacket(const RedisTask& task);

	void ProcLogin(const RedisTask& task);


private:
	RedisCpp::CRedisConn*			mConn = nullptr;
	std::thread						mThread;
	bool							mIsThreadRun = false;
	std::queue<RedisTask>			mRequestTaskPool;
	std::queue<RedisTask>			mResponseTaskPool;

	std::mutex						mRequestTaskLock;
	std::mutex						mResponseTaskLock;

	using receiver = void(Redis::*)(const RedisTask& task);
	std::unordered_map<RedisTaskID, receiver>	mRecvProcDict;

};

