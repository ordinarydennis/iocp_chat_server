#pragma once

#include <string>
#include <basetsd.h>

namespace JChat
{
	class RoomUser
	{
	public:
		RoomUser();

		~RoomUser();

		UINT32			GetClientId() const;

		std::string		GetUserId() const;

		void			SetClientId(UINT32 clientId);

		void			SetUserId(std::string userId);

	private:
		std::string			mUserId;
		UINT32				mClientId = 0;
	};
}

