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
    Error           Init(UINT16 SERVER_PORT);
    void            Run();
    void            Destroy();
    void            SendData(stPacket packet);
    void            AddToClientPoolSendPacket(ClientInfo * c);
    bool            IsEmptyClientPoolRecvPacket();
    bool            IsEmptyClientPoolSendPacket();
    ClientInfo*     GetClientInfo(UINT32 id);
    std::pair<ClientInfo*, size_t>     GetClientRecvedPacket();
    ClientInfo*     GetClientSendingPacket();

private:
    void            SetMaxThreadCount();
    Error           WinsockStartup();
    Error           CreateListenSocket();
    Error           CreateIOCP();
    void            CreateClient(const UINT32 maxClientCount);
    Error           BindandListen(UINT16 SERVER_PORT);
    Error           RegisterListenSocketToIOCP();
    Error           SetAsyncAccept();

    void            SetWokerThread();
    void            WokerThread();
    void            SetSendPacketThread();
    void            SendPacketThread();
    void            ProcAcceptOperation(stOverlappedEx* pOverlappedEx);
    void            ProcRecvOperation(ClientInfo* pClientInfo, DWORD dwIoSize);
    void            ProcSendOperation(ClientInfo* pClientInfo, DWORD dwIoSize);
    bool            SendMsg(ClientInfo* pClientInfo, char* pMsg, UINT32 len);
    void            SetAccepterThread();
    void            AccepterThread();
    void            CloseSocket(ClientInfo* pClientInfo, bool bIsForce = false);
    ClientInfo*     GetEmptyClientInfo();
    bool            BindIOCompletionPort(ClientInfo* pClientInfo);
    bool            BindRecv(ClientInfo* pClientInfo);
    void            DestroyThread();
    void            AddToClientPoolRecvPacket(ClientInfo* c, size_t size);
    BOOL            AsyncAccept(SOCKET listenSocket);

private:
    UINT16      mMaxThreadCount = 0;
    SOCKET      mListenSocket   = INVALID_SOCKET;
    HANDLE		mIOCPHandle     = INVALID_HANDLE_VALUE;
    bool		mIsWorkerRun    = true;
    bool		mIsAccepterRun  = true;
    bool        mSendPacketRun = true;
    //접속 되어있는 클라이언트 수
    int			mClientCnt = 0;
    std::thread                 mAccepterThread;
    std::vector<std::thread>    mIOWorkerThreads;
    std::vector<std::thread>    mSendPacketThreads;
    std::vector<ClientInfo>     mClientInfos;
 
    std::queue<std::pair<ClientInfo*, size_t>>     mClientPoolRecvedPacket;
    std::queue<ClientInfo*>     mClientPoolSendingPacket;

    std::mutex                  mRecvPacketLock;
    std::mutex                  mSendPacketLock;

    std::unique_ptr<stOverlappedEx> mAcceptOverlappedEx = std::make_unique<stOverlappedEx>();
};