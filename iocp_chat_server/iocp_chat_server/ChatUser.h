#pragma once

#include <basetsd.h>
#include <string>

class ChatUser
{
public:
	ChatUser() = default;

	ChatUser(const std::string userId, const UINT32 clientId);

	~ChatUser() = default;

	std::string			GetUserId()		const { return mUserId; };

	UINT32				GetClientId()	const { return mClientId; }

	UINT32				GetRoomNumber() const { return mRoomNumber; };

	void				SetRoomNumber(const UINT32 roomNumber) { mRoomNumber = roomNumber; };
		
private:
	std::string			mUserId;
	UINT32				mClientId = 0;
	UINT32				mRoomNumber = 0;
};

