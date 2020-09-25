#include "Redis.h"
#include "RedisPacket.h"
#include "Define.h"
//include 순서 중요! CRedisConn가 위로 올라가면 중복 선언 에러 발생
//Define.h 에서 winsock2 를 include CRedisConn.h 에서 winsock를 include 하기 때문에
//중복 선언 에러 발생
#include "../thirdparty/CRedisConn.h"


Redis::Redis()
	:mConn(nullptr)
{
	mConn = new RedisCpp::CRedisConn;
}
Redis::~Redis()
{
	delete mConn;
}
bool Redis::Connect(const char* ip, unsigned port)
{
	bool ret = mConn->connect(ip, port);
	return ret;
}
void Redis::Run()
{
	mThread = std::thread([this]() { RedisThread(); });
	mIsThreadRun = true;
}
void Redis::Destroy()
{
	mIsThreadRun = false;
	if (mThread.joinable())
	{
		mThread.join();
	}
}
void Redis::RedisThread()
{
	while (mIsThreadRun)
	{
		RedisTask reqTask = GetRequestTask();

		if (REDIS_TASK_ID::INVALID == reqTask.GetTaskId())
		{
			Sleep(50);
			continue;
		}

		if (REDIS_TASK_ID::REQUEST_LOGIN == reqTask.GetTaskId())
		{
			ERROR_CODE error_code = ERROR_CODE::LOGIN_USER_INVALID_PW;

			LoginReqRedisPacket reqPacket(reqTask);
			std::string value;
			if (mConn->get(reqPacket.GetUserIdstr(), value))
			{
				if (value.compare(reqPacket.GetUserPw()) == 0)
				{
					error_code = ERROR_CODE::NONE;
				}
			}

			LoginResRedisPacket resPacket(reqPacket.GetUserId(), REDIS_TASK_ID::RESPONSE_LOGIN, reinterpret_cast<char*>(&error_code), sizeof(error_code));
			ResponseTask(resPacket.GetTask());
		}
	}
}
void Redis::RequestTask(const RedisTask& task)
{
	std::lock_guard<std::mutex> guard(mRequestTaskLock);
	mRequestTaskPool.push(task);
}
void Redis::ResponseTask(const RedisTask& task)
{
	std::lock_guard<std::mutex> guard(mResponseTaskLock);
	mResponseTaskPool.push(task);
}
RedisTask Redis::GetRequestTask()
{
	std::lock_guard<std::mutex> guard(mRequestTaskLock);
	if (mRequestTaskPool.empty())
	{
		return RedisTask();
	}
	
	RedisTask task = mRequestTaskPool.front();
	mRequestTaskPool.pop();
	return task;
}
RedisTask Redis::GetResponseTask()
{
	std::lock_guard<std::mutex> guard(mResponseTaskLock);
	if (mResponseTaskPool.empty())
	{
		return RedisTask();
	}
	
	RedisTask task = mResponseTaskPool.front();
	mResponseTaskPool.pop();
	return task;
}
