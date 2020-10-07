#pragma once

#include "Network.h"
#include <basetsd.h>
#include <list>
#include <functional>

class ChatUser;

//TODO 최흥배
// 코드 일관성을 위해 GetRoomNumber() 처럼 1라인으로 끝나는 함수는 헤더 파일에 정의를 할지 아니면 다른 클래스처럼 함수 정의는 cpp에 할지 일관성 있게 했으면 좋겠습니다.
// 저는 1라인으로 끝나는 것은 헤더 파일에 정의하는 것을 추천합니다.
class Room
{
public:
	UINT32 GetRoomNumber() { return mRoomNumber; };
	std::list<ChatUser*>* GetUserList() { return &mUserList; };
	void SetRoomNumber(UINT32 roomNumber);
	void AddUser(ChatUser* chatUser);
	void RemoveUser(ChatUser* chatUser);

	//TODO 최흥배
	// 이 함수를 호출하는 곳에서 매번 std::function<void(stPacket)>을 만들어서 넘기고 있습니다.
	// 호출하는 곳에서 한번만 만들어서 참조로 전달하거나 혹은 여기에 std::function<void(stPacket)>을 static으로 만들어서 매번 객체를 생성하지 않도록 합니다.
	void Notify(UINT32 clientFrom, UINT16 packetId, const char* body, size_t bodySize, std::function<void(stPacket)> packetSender);

private:
	UINT32						mRoomNumber = 0;

	//TODO 최흥배
	//방에 들어가는 유저수가 아주 많지 않다면 vector 사용을 추천합니다. 
	// 자료구조 측면에서는 list가 맞지만 저장수가 많지 않다면 vector가 사용하기 편하고 성능적 측면의 문제도 없습니다.
	// 오히려 메모리 할당을 줄이고 캐시에 좋아서 더 좋을 수도 있습니다.
	std::list<ChatUser*>		mUserList;
};

