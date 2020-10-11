#include "ChatUser.h"
#include "ClientInfo.h"

ChatUser::ChatUser(const std::string userId, const UINT32 clientId)
{
	mUserId = userId;
	mClientId = clientId;
}