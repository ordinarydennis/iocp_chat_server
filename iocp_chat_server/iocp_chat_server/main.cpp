#include "ChatServer.h"

//TODO 최흥배 
// 변경될 수 있는 설정 값은 서버 실행 시 외부에서 받도록 합니다
// https://jacking75.github.io/C++_argument_parser_flags/  이 라이브러리를 사용해보죠

//TODO 최흥배
// const를 적극 사용해봅니다

//TODO 최흥배
// 중괄호({)를 다 사용합니다. if 문 등에서

//TODO 최흥배
// 사용하지 않는 코드는 꼭 삭제 부탁합니다
// 앞으로는 제가 코드 리뷰할 때는 그전에 코드 정리 꼭 부탁합니다
// 언제나 깨끗한 코드를 유지하기 바랍니다

int main()
{
	ChatServer chatServer;

	Error error = Error::NONE;
	error = chatServer.Init();
	if (Error::NONE != error)
	{
		printf("[ERROR] Error Number: %d, Get Last Error: %d\n", error, WSAGetLastError());
		return static_cast<int>(error);
	}

	chatServer.Run();
	chatServer.Destroy();

	return static_cast<int>(Error::NONE);
}

