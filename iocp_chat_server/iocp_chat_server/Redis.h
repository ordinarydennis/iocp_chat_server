#pragma once

#include "RedisTask.h"
#include "Define.h"
#include <vector>
#include <thread>
#include <queue>
#include <memory>
#include <mutex>

namespace RedisCpp
{
	class CRedisConn;
}

class Redis
{
public:
	Redis();
	~Redis();

	Error Connect(const char* ip, unsigned port);
	void Run();
	void Destroy();

	void RequestTask(const RedisTask& task);
	RedisTask GetResponseTask();

private:
	void ResponseTask(const RedisTask& task);
	RedisTask GetRequestTask();
	void RedisThread();

private:
	RedisCpp::CRedisConn*			mConn = nullptr;
	std::thread						mThread;
	bool							mIsThreadRun = false;
	std::queue<RedisTask>			mRequestTaskPool;
	std::queue<RedisTask>			mResponseTaskPool;

	std::mutex						mRequestTaskLock;
	std::mutex						mResponseTaskLock;
};

