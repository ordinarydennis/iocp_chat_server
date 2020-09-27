#include "ChatUserManager.h"
#include "ClientInfo.h"

ChatUserManager::ChatUserManager()
{
}
ChatUserManager::~ChatUserManager()
{

}
ChatUser* ChatUserManager::GetUser(const UINT32 userId)
{
	auto item = mChatUserDict.find(userId);
	bool isExist =  (item != mChatUserDict.end()) ? true : false;

	if (false == isExist)
	{
		return nullptr;
	}

	return &mChatUserDict[userId];
}
void ChatUserManager::AddUser(const ChatUser& chatUser)
{
	mChatUserDict[chatUser.GetClientInfo()->GetId()] = chatUser;
}
