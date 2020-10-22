#pragma once

#include <basetsd.h>
#include <string>

namespace JChat
{
	class ChatUser
	{
	public:
		enum class STATE
		{
			NONE,
			LOGIN,
			ENTER_ROOM,
		};

	public:
		ChatUser() = default;

		ChatUser(const std::string userId, const UINT32 clientId);

		~ChatUser() = default;

		std::string			GetUserId() const;

		UINT32				GetClientId() const;

		UINT32				GetRoomNumber() const;

		void				SetRoomNumber(const UINT32 roomNumber);

		void				SetState(STATE state);

		STATE				GetState();

	private:
		std::string			mUserId;
		UINT32				mClientId = 0;
		UINT32				mRoomNumber = 0;
		STATE				mState = STATE::NONE;
	};
}

