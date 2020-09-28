#include "ChatUser.h"
#include "ClientInfo.h"

ChatUser::ChatUser(std::string userId, ClientInfo* clientInfo)
{
	mUserId = userId;
	mClientInfo = clientInfo;
}
ChatUser::~ChatUser()
{
	
}
std::string ChatUser::GetUserId() const
{
	return mUserId;
}
UINT32 ChatUser::GetClientId()
{
	return mClientInfo->GetId();
}
void ChatUser::SetRoom(Room* room)
{
	mRoom = room;
}