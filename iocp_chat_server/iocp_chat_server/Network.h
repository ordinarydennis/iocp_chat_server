#pragma once

#include "Define.h"
#include "ClientInfo.h"
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
    std::thread                 mAccepterThread;
    std::vector<std::thread>    mIOWorkerThreads;
    std::vector<ClientInfo>     mClientInfos;
 
    std::queue<ClientInfo*>     mClientPoolRecvedPacket;
    std::queue<ClientInfo*>     mClientPoolSendingPacket;

    std::mutex                  mRecvPacketLock;
    std::mutex                  mSendPacketLock;

    std::unique_ptr<stOverlappedEx> mAcceptOverlappedEx = std::make_unique<stOverlappedEx>();
  
private:
    void CreateListenSocket();
    void BindandListen();
    void CreateIOCP();
    void CreateClient(const UINT32 maxClientCount);

    void SetWokerThread();
    void WokerThread();
    void ProcAcceptOperation(stOverlappedEx* pOverlappedEx);
    void ProcRecvOperation(ClientInfo* pClientInfo, DWORD dwIoSize);
    void ProcSendOperation(ClientInfo* pClientInfo, DWORD dwIoSize);
    bool SendMsg(ClientInfo* pClientInfo, char* pMsg, UINT32 len);
    void SetAccepterThread();
    void AccepterThread();
    void CloseSocket(ClientInfo* pClientInfo, bool bIsForce = false);
    ClientInfo* GetEmptyClientInfo();
    ClientInfo* GetClientInfo(UINT32 id);
    bool BindIOCompletionPort(ClientInfo* pClientInfo);
    bool BindRecv(ClientInfo* pClientInfo);
    void DestroyThread();
    void AddToClientPoolRecvPacket(ClientInfo* c);
    BOOL AsyncAccept(SOCKET listenSocket);

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
    ClientInfo* GetClientRecvedPacket();
    
    bool IsEmptyClientPoolSendPacket();
    ClientInfo* GetClientSendingPacket();
    
    void SendData(stPacket packet);
    void AddToClientPoolSendPacket(ClientInfo* c);

    // 10. getter/setter 멤버함수들 선언
public:

};