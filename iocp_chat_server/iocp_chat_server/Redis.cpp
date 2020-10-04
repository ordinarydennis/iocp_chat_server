#include "Redis.h"
#include "RedisPacket.h"
#include "Define.h"
//include 순서 중요! CRedisConn가 위로 올라가면 중복 선언 에러 발생
//Define.h 에서 winsock2 를 include CRedisConn.h 에서 winsock를 include 하기 때문에
//중복 선언 에러 발생
#include "../thirdparty/CRedisConn.h"
#include <sstream>


Redis::Redis()
	:mConn(nullptr)
{
	mConn = new RedisCpp::CRedisConn;
}

Redis::~Redis()
{
	delete mConn;
}

Error Redis::Connect(const char* ip, unsigned port)
{
	bool ret = mConn->connect(ip, port);
	if (false == ret)
		return Error::REDIS_CONNECT;

	return Error::NONE;
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

			std::string pw;
			if (mConn->get(reqPacket.GetUserId(), pw))
			{
				if (pw.compare(reqPacket.GetUserPw()) == 0)
				{
					error_code = ERROR_CODE::NONE;
				}

				const size_t bufSize = MAX_USER_ID_BYTE_LENGTH + sizeof(ERROR_CODE);
				char buf[bufSize] = { 0, };
				if (ERROR_CODE::NONE == error_code)
				{
					memcpy_s(buf, strlen(reqPacket.GetUserId()), reqPacket.GetUserId(), strlen(reqPacket.GetUserId()));
				}
				memcpy_s(&buf[MAX_USER_ID_BYTE_LENGTH], sizeof(error_code), &error_code, sizeof(error_code));
				LoginResRedisPacket resPacket(reqPacket.GetClientId(), REDIS_TASK_ID::RESPONSE_LOGIN, buf);
				ResponseTask(resPacket.GetTask());
			}

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
