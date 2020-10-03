#include "ChatUser.h"
#include "ClientInfo.h"

ChatUser::ChatUser(std::string userId, UINT32 clientId)
{
	mUserId = userId;
	mClientId = clientId;
}

void ChatUser::SetRoomNumber(UINT32 roomNumber)
{
	mRoomNumber = roomNumber;
}