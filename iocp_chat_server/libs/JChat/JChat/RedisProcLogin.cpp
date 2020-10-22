#include "PacketProcessor.h"
#include "ChatUserManager.h"
#include "RedisPacket.h"

namespace JChat
{
	void PacketProcessor::RedisProcLogin(const JNet::RedisTask& task)
	{
		JNet::LoginResRedisPacket loginResRedisPacket;
		loginResRedisPacket.DecodeTask(task);

		JCommon::ResultResPacket resultResPacket;
		resultResPacket.mResult = loginResRedisPacket.mResult;

		SendPacket(
			task.mClientId,
			task.mClientId,
			static_cast<UINT16>(JCommon::PACKET_ID::LOGIN_RES),
			reinterpret_cast<char*>(&resultResPacket),
			sizeof(resultResPacket)
		);

		if (JCommon::CLIENT_ERROR_CODE::NONE == loginResRedisPacket.mResult)
		{
			ChatUser chatUser(loginResRedisPacket.mUserId, task.mClientId);
			chatUser.SetState(ChatUser::STATE::LOGIN);
			mChatUserManager->AddUser(chatUser);
		}
	}
}