#include "ChatUser.h"
#include "ClientInfo.h"

ChatUser::ChatUser(std::string user_id, ClientInfo* clientInfo)
{
	mUserId = user_id;
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