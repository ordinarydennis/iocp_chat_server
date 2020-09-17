#pragma once

#include "Define.h"
#include <vector>
#include <thread>

class ChatServer
{
    // 1. 필요시 static_assert

    // 2. 매크로 집단

    // 3. friend 클래스가 있다면 선언

    // 4. 해당 class에 종속적인 타입별칭이 필요하다면, 변수 선언에 앞서 미리 정의

    // 5. 멤버변수 선언
private:
    std::thread	                mReceivePacketThread;
    bool                        mReceivePacketRun = true;

private:
    ChatServer() = default;

    void SetReceivePacketThread();
    void ReceivePacketThread();
    void Waiting();

    // 6. 생성자/소멸자 선언
public:
    ~ChatServer() { WSACleanup(); };
    ChatServer(const ChatServer&) = delete;
    ChatServer(ChatServer&&) = delete;
    ChatServer& operator=(const ChatServer&) = delete;
    ChatServer& operator=(ChatServer&&) = delete;

    // 7. 정적 멤버함수들 선언
public:
    static ChatServer& Instance()
    {
        static ChatServer instance;
        return instance;
    }

    // 8. 가상 멤버함수들 선언
public:

    // 9. 일반 멤버함수들 선언
public:
    void Init();
    void Run();
    void Destroy();

    // 10. getter/setter 멤버함수들 선언
public:

};

#define ChatServerInstance ChatServer::Instance()
