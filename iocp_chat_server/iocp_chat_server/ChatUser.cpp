#include "ChatUser.h"
#include "ClientInfo.h"

ChatUser::ChatUser(const std::string userId, const UINT32 clientId)
{
	mUserId = userId;
	mClientId = clientId;
}

std::string ChatUser::GetUserId() const
{
	return mUserId;
}

UINT32 ChatUser::GetClientId() const
{
	return mClientId;
}

UINT32 ChatUser::GetRoomNumber() const
{
	return mRoomNumber;
};

void ChatUser::SetRoomNumber(const UINT32 roomNumber)
{
	mRoomNumber = roomNumber;
};
