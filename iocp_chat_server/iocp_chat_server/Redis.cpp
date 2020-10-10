#include "Redis.h"
#include "RedisPacket.h"
#include "Define.h"
//include 순서 중요! CRedisConn가 위로 올라가면 중복 선언 에러 발생
//Define.h 에서 winsock2 를 include CRedisConn.h 에서 winsock를 include 하기 때문에
//중복 선언 에러 발생
#include "../thirdparty/CRedisConn.h"
#include "RedisProcessor.h"
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

Error Redis::Connect(const char* ip, unsigned port)
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
		//TODO 최흥배
		// std::nullopt 로 비교해야 좋을 것 같습니다. GetRequestTask() 에서 std::nullopt를 반환하니 
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
	//TODO 최흥배
	//장래에 복수의 case에서도 처리될 수 있도록 구조화 하기 바랍니다. 패킷 처리쪽처럼
	ERROR_CODE error_code = ERROR_CODE::LOGIN_USER_INVALID_PW;

	LoginReqRedisPacket reqPacket(task);
	const size_t bufSize = MAX_USER_ID_BYTE_LENGTH + sizeof(ERROR_CODE);
	char buf[bufSize] = { 0, };
	std::string pw;
	if (mConn->get(reqPacket.GetUserId(), pw))
	{
		if (pw.compare(reqPacket.GetUserPw()) == 0)
		{
			memcpy_s(buf, strnlen_s(reqPacket.GetUserId(), MAX_USER_PW_BYTE_LENGTH), reqPacket.GetUserId(), strnlen_s(reqPacket.GetUserId(), MAX_USER_PW_BYTE_LENGTH));
			error_code = ERROR_CODE::NONE;
		}
	}
	memcpy_s(&buf[MAX_USER_ID_BYTE_LENGTH], sizeof(error_code), &error_code, sizeof(error_code));
	LoginResRedisPacket resPacket(reqPacket.GetClientId(), RedisTaskID::RESPONSE_LOGIN, buf);
	ResponseTask(resPacket.GetTask());

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