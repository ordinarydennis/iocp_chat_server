#pragma once

#include "Define.h"
#include <thread>
#include <vector>
#include <queue>
#include <mutex>

class Network
{
    // 1. 필요시 static_assert

    // 2. 매크로 집단

    // 3. friend 클래스가 있다면 선언

    // 4. 해당 class에 종속적인 타입별칭이 필요하다면, 변수 선언에 앞서 미리 정의

    // 5. 멤버변수 선언
private:
    SOCKET      mListenSocket   = INVALID_SOCKET;
    HANDLE		mIOCPHandle     = INVALID_HANDLE_VALUE;
    bool		mIsWorkerRun    = true;
    bool		mIsAccepterRun  = true;
    //접속 되어있는 클라이언트 수
    int			mClientCnt = 0;
    std::thread	                mAccepterThread;
    std::vector<std::thread>    mIOWorkerThreads;
    std::vector<stClientInfo>   mClientInfos;
    //트리거가 된 유저들은 유저풀에 큐잉한다.
    std::queue<stClientInfo*>    mClientPoolRecvPacket;
    std::queue<stClientInfo*>    mClientPoolSendPacket;
    
    
    stPacket                    mSendingPacket;     //현재 전송중인 패킷

private:
    void CreateSocket();
    void BindandListen();
    void CreateIOCP();
    void CreateClient(const UINT32 maxClientCount);
    void SetWokerThread();
    void WokerThread();
    bool SendMsg(stClientInfo * pClientInfo, char* pMsg, int nLen);
    void SetAccepterThread();
    void AccepterThread();
    void CloseSocket(stClientInfo* pClientInfo, bool bIsForce = false);
    stClientInfo* GetEmptyClientInfo();
    bool BindIOCompletionPort(stClientInfo * pClientInfo);
    bool BindRecv(stClientInfo * pClientInfo);
    void DestroyThread();

    // 6. 생성자/소멸자 선언
public:
    Network() = default;
    ~Network() { WSACleanup(); };
    Network(const Network&) = delete;
    Network(Network&&) = delete;
    Network& operator=(const Network&) = delete;
    Network& operator=(Network&&) = delete;

    // 7. 정적 멤버함수들 선언
public:

    // 8. 가상 멤버함수들 선언
public:

    // 9. 일반 멤버함수들 선언
public:
    void Init();
    void Run();
    void Destroy();

    bool IsEmptyClientPoolRecvPacket();
    stClientInfo* GetClientReceivedPacket();
    
    bool IsEmptyClientPoolSendPacket();
    stClientInfo* GetClientSendPacket();
    
    void SendData(stPacket packet);
    void AddClient(stClientInfo* c);

    // 10. getter/setter 멤버함수들 선언
public:

};