#pragma once

#include <basetsd.h>
#include <string>

class ChatUser
{
public:
	ChatUser() = default;
	//TODO 최흥배
	// 코드에 일관성이 있어야 합니다. 코딩 규칙에 맞지 않습니다.
	ChatUser(std::string userId, UINT32 clientId);
	~ChatUser() = default;

	std::string			GetUserId()		const { return mUserId; };
	UINT32				GetClientId()	const { return mClientId; }
	UINT32				GetRoomNumber() const { return mRoomNumber; };

	void				SetRoomNumber(UINT32 roomNumber);
		
private:
	std::string			mUserId;
	//TODO 최흥배
	// 클래스 디자인 측면에서 ChatUser가 ClientInfo와 Room 객체를 참조하여 연결관계가 만들어졌는데 서로간에 독립성을 지켜주었으면 합니다.
	UINT32				mClientId = 0;
	UINT32				mRoomNumber = 0;
};

