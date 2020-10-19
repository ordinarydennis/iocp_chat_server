#include "PacketProcessor.h"
#include "../../JNet/JNet/RedisPacket.h"
#include "../../JNet/JNet/Redis.h"

namespace JChat
{
	void PacketProcessor::ProcLogin(const JCommon::stPacket& packet)
	{
		const JCommon::LoginReqPacket* loginReqPacket = reinterpret_cast<const JCommon::LoginReqPacket*>(&packet.mBody);

		loginReqPacket->mUserId;
		loginReqPacket->mUserPw;

		printf("Login User Id : %s passwd : %s \n", loginReqPacket->mUserId, loginReqPacket->mUserPw);

		JNet::LoginReqRedisPacket redisReqPacket;
		redisReqPacket.mClientId = packet.mClientFrom;
		redisReqPacket.mRedisTaskId = JNet::REDIS_TASK_ID::REQUEST_LOGIN;
		size_t userIdSize = strnlen_s(loginReqPacket->mUserId, JCommon::MAX_USER_ID_BYTE_LENGTH);
		memcpy_s(redisReqPacket.mUserId, userIdSize, loginReqPacket->mUserPw, userIdSize);
		size_t userPwSize = strnlen_s(loginReqPacket->mUserId, JCommon::MAX_USER_PW_BYTE_LENGTH);
		memcpy_s(redisReqPacket.mUserPw, userPwSize, loginReqPacket->mUserPw, userPwSize);

		mRedis->RequestTask(redisReqPacket.EncodeTask());
	}
}

