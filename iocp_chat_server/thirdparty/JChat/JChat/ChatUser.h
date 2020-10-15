#pragma once

#include <basetsd.h>
#include <string>

namespace JChat
{
	class ChatUser
	{
	public:
		ChatUser() = default;

		ChatUser(const std::string userId, const UINT32 clientId);

		~ChatUser() = default;

		std::string			GetUserId() const;

		UINT32				GetClientId() const;

		UINT32				GetRoomNumber() const;

		void				SetRoomNumber(const UINT32 roomNumber);

	private:
		std::string			mUserId;
		UINT32				mClientId = 0;
		UINT32				mRoomNumber = 0;
	};
}

