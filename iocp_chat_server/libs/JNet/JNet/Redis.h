#pragma once

#include "Define.h"
#include "RedisDefine.h"
#include "SQueue.h"
#include <thread>
#include <queue>
#include <mutex>
#include <unordered_map>
#include <optional>

namespace RedisCpp
{
	class CRedisConn;
}

namespace JNet
{
	class Redis
	{
	public:
		Redis();

		~Redis();

		JCommon::ERROR_CODE		Connect(const char* ip, const UINT16 port);

		void			Run();

		void			Destroy();

		void			RequestTask(const RedisTask& task);

		std::optional<RedisTask>	GetResponseTask();

		std::optional<RedisTask> GetRequestTask();

	private:
		void			ResponseTask(const RedisTask& task);

		void			RedisThread();

		void			ProcessRedisPacket(const RedisTask& task);

		void			ProcLogin(const RedisTask& task);

	private:
		RedisCpp::CRedisConn*			mConn = nullptr;
		std::thread						mThread;
		bool							mIsThreadRun = false;

		using receiver = void(Redis::*)(const RedisTask& task);
		std::unordered_map<REDIS_TASK_ID, receiver>	mRecvProcDict;

		JNet::SQueue<EntryRedisTask>	mRequestTaskQueue;
		JNet::SQueue<EntryRedisTask>	mResponseTaskQueue;
	};
}

