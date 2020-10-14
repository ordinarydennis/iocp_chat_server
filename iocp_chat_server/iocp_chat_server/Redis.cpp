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
	mRecvProcDict[RedisTaskID::REQUEST_LOGIN] = &Redis::ProcLogin;
}

Redis::~Redis()
{
	delete mConn;
	mConn = nullptr;
}

Error Redis::Connect(const char* ip, const UINT16 port)
{
	bool ret = mConn->connect(ip, port);
	if (false == ret)
	{
		return Error::REDIS_CONNECT;
	}

	return Error::NONE;
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
	auto iter = mRecvProcDict.find(task.GetTaskId());
	if (iter != mRecvProcDict.end())
	{
		(this->*(iter->second))(task);
	}
}

void Redis::ProcLogin(const RedisTask& task)
{
	LoginReqRedisPacket reqPacket;
	reqPacket.DecodeTask(task);
	
	ERROR_CODE error_code = ERROR_CODE::LOGIN_USER_INVALID_PW;

	LoginResRedisPacket resPacket;
	std::string pw;
	
	if (mConn->get(reqPacket.mUserId, pw))
	{
		if (pw.compare(reqPacket.mUserPw) == 0)
		{
			size_t userIdSize = strnlen_s(reqPacket.mUserId, MAX_USER_PW_BYTE_LENGTH);
			memcpy_s(resPacket.mUserId, userIdSize, reqPacket.mUserId, userIdSize);
			error_code = ERROR_CODE::NONE;
		}
	}

	resPacket.mClientId = reqPacket.mClientId;
	resPacket.mRedisTaskId = RedisTaskID::RESPONSE_LOGIN;;
	resPacket.mResult = error_code;

	ResponseTask(resPacket.EncodeTask());
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

std::optional<RedisTask> Redis::GetRequestTask()
{
	std::lock_guard<std::mutex> guard(mRequestTaskLock);
	if (mRequestTaskPool.empty())
	{
		return std::nullopt;
	}
	
	RedisTask task = mRequestTaskPool.front();
	mRequestTaskPool.pop();
	return task;
}

std::optional<RedisTask> Redis::GetResponseTask()
{
	std::lock_guard<std::mutex> guard(mResponseTaskLock);
	if (mResponseTaskPool.empty())
	{
		return std::nullopt;
	}
	
	RedisTask task = mResponseTaskPool.front();
	mResponseTaskPool.pop();
	return task;
}

void Redis::Destroy()
{
	mIsThreadRun = false;
	if (mThread.joinable())
	{
		mThread.join();
	}
}