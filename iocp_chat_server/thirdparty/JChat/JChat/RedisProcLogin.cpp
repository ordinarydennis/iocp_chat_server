#include "PacketProcessor.h"
#include "ChatUserManager.h"
#include "../../JNet/JNet/RedisPacket.h"

namespace JChat
{
	void PacketProcessor::RedisProcLogin(const JNet::RedisTask& task)
	{
		JNet::LoginResRedisPacket loginResRedisPacket;
		loginResRedisPacket.DecodeTask(task);

		JCommon::ResultResPacket resultResPacket;
		resultResPacket.mResult = loginResRedisPacket.mResult;

		SendPacket(
			task.GetClientId(),
			task.GetClientId(),
			static_cast<UINT16>(JCommon::PACKET_ID::LOGIN_RES),
			reinterpret_cast<char*>(&resultResPacket),
			sizeof(resultResPacket)
		);

		if (JCommon::CLIENT_ERROR_CODE::NONE == loginResRedisPacket.mResult)
		{
			ChatUser chatUser(loginResRedisPacket.mUserId, task.GetClientId());
			mChatUserManager->AddUser(chatUser);
		}
	}
}