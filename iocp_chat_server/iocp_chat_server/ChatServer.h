#pragma once

#include "Define.h"
#include <vector>
#include <thread>
#include <memory>
#include "Network.h"

class ChatServer
{
    // 1. 필요시 static_assert

    // 2. 매크로 집단

    // 3. friend 클래스가 있다면 선언

    // 4. 해당 class에 종속적인 타입별칭이 필요하다면, 변수 선언에 앞서 미리 정의

    // 5. 멤버변수 선언
private:
    std::thread	                mReceivePacketThread;
    //std::thread	                mSendPacketThread;
    std::vector<std::thread>    mSendPacketThreads;
    bool                        mReceivePacketRun = true;
    bool                        mSendPacketRun = true;
    std::unique_ptr<Network>    mNetwork;

private:
    void SetReceivePacketThread();
    void ReceivePacketThread();
    void SetSendPacketThread();
    void SendPacketThread();
    void Waiting();

    // 6. 생성자/소멸자 선언
public:
    ChatServer() = default;
    ~ChatServer() { WSACleanup(); };
    ChatServer(const ChatServer&) = delete;
    ChatServer(ChatServer&&) = delete;
    ChatServer& operator=(const ChatServer&) = delete;
    ChatServer& operator=(ChatServer&&) = delete;

    // 7. 정적 멤버함수들 선언
public:

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
