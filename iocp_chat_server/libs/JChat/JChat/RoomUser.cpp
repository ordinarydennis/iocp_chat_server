#include "RoomUser.h"

namespace JChat
{
	RoomUser::RoomUser()
	{

	}

	RoomUser::~RoomUser()
	{

	}

	UINT32 RoomUser::GetClientId() const
	{
		return mClientId;
	}

	std::string RoomUser::GetUserId() const
	{
		return mUserId;
	}

	void RoomUser::SetClientId(UINT32 clientId)
	{
		mClientId = clientId;
	}

	void RoomUser::SetUserId(std::string userId)
	{
		mUserId = userId;
	}
}