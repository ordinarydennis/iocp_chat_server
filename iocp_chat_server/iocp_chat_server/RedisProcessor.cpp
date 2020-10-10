#include "RedisProcessor.h"

RedisProcessor::RedisProcessor()
{
	mRecvProcDict[RedisTaskID::RESPONSE_LOGIN] = &RedisProcessor::ProcLogin;
}

RedisProcessor::~RedisProcessor()
{

}

void RedisProcessor::ProcessRedisPacket(const RedisTask& task)
{
	auto iter = mRecvProcDict.find(task.GetTaskId());
	if (iter != mRecvProcDict.end())
	{
		(this->*(iter->second))(task);
	}
}

void RedisProcessor::ProcLogin(const RedisTask& task)
{
	////TODO 최흥배
	////장래에 복수의 case에서도 처리될 수 있도록 구조화 하기 바랍니다. 패킷 처리쪽처럼
	//ERROR_CODE error_code = ERROR_CODE::LOGIN_USER_INVALID_PW;

	//LoginReqRedisPacket reqPacket(task);
	//const size_t bufSize = MAX_USER_ID_BYTE_LENGTH + sizeof(ERROR_CODE);
	//char buf[bufSize] = { 0, };
	//std::string pw;
	//if (mConn->get(reqPacket.GetUserId(), pw))
	//{
	//	//TODO 최흥배
	//	// 아래 2개의 if문은 하나로 합치죠
	//	if (pw.compare(reqPacket.GetUserPw()) == 0)
	//	{
	//		memcpy_s(buf, strnlen_s(reqPacket.GetUserId(), MAX_USER_PW_BYTE_LENGTH), reqPacket.GetUserId(), strnlen_s(reqPacket.GetUserId(), MAX_USER_PW_BYTE_LENGTH));
	//		error_code = ERROR_CODE::NONE;
	//	}
	//}
	//memcpy_s(&buf[MAX_USER_ID_BYTE_LENGTH], sizeof(error_code), &error_code, sizeof(error_code));
	//LoginResRedisPacket resPacket(reqPacket.GetClientId(), RedisTaskID::RESPONSE_LOGIN, buf);
	//ResponseTask(resPacket.GetTask());
}