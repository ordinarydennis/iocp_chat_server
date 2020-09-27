#pragma once

#include "Define.h"
#include "ClientInfo.h"
#include <thread>
#include <vector>
#include <queue>
#include <mutex>

class Network
{
public:
    Network() = default;
    ~Network() { WSACleanup(); };
    //Network(const Network&) = delete;
    //Network(Network&&) = delete;
    //Network& operator=(const Network&) = delete;
    //Network& operator=(Network&&) = delete;

    void Init(UINT16 SERVER_PORT);
    void Run();
    void Destroy();

    bool IsEmptyClientPoolRecvPacket();
    ClientInfo* GetClientRecvedPacket();

    bool IsEmptyClientPoolSendPacket();
    ClientInfo* GetClientSendingPacket();

    void SendData(stPacket packet);
    void AddToClientPoolSendPacket(ClientInfo * c);

    ClientInfo* GetClientInfo(UINT32 id);

private:
    void CreateListenSocket();
    void BindandListen(UINT16 SERVER_PORT);
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
    bool BindIOCompletionPort(ClientInfo* pClientInfo);
    bool BindRecv(ClientInfo* pClientInfo);
    void DestroyThread();
    void AddToClientPoolRecvPacket(ClientInfo* c);
    BOOL AsyncAccept(SOCKET listenSocket);

private:
    UINT16      mMaxThreadCount = 0;
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
 
};