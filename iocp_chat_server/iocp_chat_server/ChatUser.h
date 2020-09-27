#pragma once

#include <basetsd.h>
#include <string>

class ClientInfo;
class Room;

class ChatUser
{
public:
	ChatUser() = default;
	ChatUser(std::string user_id, ClientInfo* clientInfo);
	~ChatUser();

	std::string GetUserId() const;
	UINT32 GetClientId();
	ClientInfo* GetClientInfo() const { return mClientInfo; };
	Room* GetRoom() const { return mRoom; };

	void SetRoom(Room* room);

private:
	std::string			mUserId;
	ClientInfo*			mClientInfo = nullptr;
	Room*				mRoom = nullptr;
};

