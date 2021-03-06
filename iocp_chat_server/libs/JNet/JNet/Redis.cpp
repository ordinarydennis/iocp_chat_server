#include "Redis.h"
#include "RedisPacket.h"
#include "CRedisConn.h"

namespace JNet
{
	Redis::Redis()
		:mConn(nullptr)
	{
		mConn = new RedisCpp::CRedisConn;
		mRecvProcDict[REDIS_TASK_ID::REQUEST_LOGIN] = &Redis::ProcLogin;
	}

	Redis::~Redis()
	{
		delete mConn;
		mConn = nullptr;
	}

	JCommon::ERROR_CODE Redis::Connect(const char* ip, const UINT16 port)
	{
		bool ret = mConn->connect(ip, port);
		if (false == ret)
		{
			return JCommon::ERROR_CODE::REDIS_CONNECT;
		}

		return JCommon::ERROR_CODE::NONE;
	}

	void Redis::Run()
	{
		mThread = std::thread([this]() { RedisThread(); });
		mIsThreadRun = true;
	}

	void Redis::RedisThread()
	{
		while (mIsThreadRun)
		{
			auto reqTaskOpt = GetRequestTask();
			if (std::nullopt == reqTaskOpt)
			{
				Sleep(1);
				continue;
			}

			ProcessRedisPacket(reqTaskOpt.value());
		}
	}

	void Redis::ProcessRedisPacket(const RedisTask& task)
	{
		auto iter = mRecvProcDict.find(task.mTaskID);
		if (iter != mRecvProcDict.end())
		{
			(this->*(iter->second))(task);
		}
	}

	void Redis::ProcLogin(const RedisTask& task)
	{
		LoginReqRedisPacket reqPacket;
		reqPacket.DecodeTask(task);

		JCommon::CLIENT_ERROR_CODE error_code = JCommon::CLIENT_ERROR_CODE::LOGIN_USER_INVALID_PW;

		LoginResRedisPacket resPacket;
		std::string pw;

		if (mConn->get(reqPacket.mUserId, pw))
		{
			if (pw.compare(reqPacket.mUserPw) == 0)
			{
				size_t userIdSize = strnlen_s(reqPacket.mUserId, JCommon::MAX_USER_PW_BYTE_LENGTH);
				memcpy_s(resPacket.mUserId, userIdSize, reqPacket.mUserId, userIdSize);
				error_code = JCommon::CLIENT_ERROR_CODE::NONE;
			}
		}

		resPacket.mClientId = reqPacket.mClientId;
		resPacket.mRedisTaskId = REDIS_TASK_ID::RESPONSE_LOGIN;;
		resPacket.mResult = error_code;

		ResponseTask(resPacket.EncodeTask());
	}

	void Redis::RequestTask(const RedisTask& task)
	{
		EntryRedisTask entry;
		entry.mTask = task;
		mRequestTaskQueue.Push(entry);
	}

	void Redis::ResponseTask(const RedisTask& task)
	{
		EntryRedisTask entry;
		entry.mTask = task;
		mResponseTaskQueue.Push(entry);
	}

	std::optional<RedisTask> Redis::GetRequestTask()
	{
		auto task = mRequestTaskQueue.Front();
		if (nullptr == task)
		{
			return std::nullopt;
		}
		auto retTask = task->mTask;
		mRequestTaskQueue.Pop();
		return retTask;
	}

	std::optional<RedisTask> Redis::GetResponseTask()
	{
		auto task = mResponseTaskQueue.Front();
		if (nullptr == task)
		{
			return std::nullopt;
		}
		auto retTask = task->mTask;
		mResponseTaskQueue.Pop();
		return retTask;
	}

	void Redis::Destroy()
	{
		mIsThreadRun = false;
		if (mThread.joinable())
		{
			mThread.join();
		}
	}
}