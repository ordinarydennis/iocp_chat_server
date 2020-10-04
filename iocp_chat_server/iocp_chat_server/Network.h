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
    void            AddToClientPoolSendPacket(ClientInfo * clientInfo);
    bool            IsEmptyClientPoolRecvPacket();
    bool            IsEmptyClientPoolSendPacket();
    ClientInfo*     GetClientInfo(UINT32 id);
    std::pair<ClientInfo*, size_t>     GetClientRecvedPacket();
    ClientInfo*     GetClientSendingPacket();
    void            SendPacket(const stPacket& packet);

private:
    void            SetMaxThreadCount();
    Error           WinsockStartup();
    Error           CreateListenSocket();
    Error           CreateIOCP();
    void            CreateClient(const UINT32 maxClientCount);
    Error           BindandListen(UINT16 SERVER_PORT);
    Error           RegisterListenSocketToIOCP();

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
    //ClientInfo*     GetEmptyClientInfo();
    bool            BindIOCompletionPort(ClientInfo* pClientInfo);
    bool            BindRecv(ClientInfo* pClientInfo);
    void            DestroyThread();
    void            AddToClientPoolRecvPacket(ClientInfo* c, size_t size);

private:
    UINT16                      mMaxThreadCount = 0;
    SOCKET                      mListenSocket   = INVALID_SOCKET;
    HANDLE                  	mIOCPHandle     = INVALID_HANDLE_VALUE;
    bool	                   	mIsWorkerRun    = true;
    bool	                   	mIsAccepterRun  = true;
    bool                        mSendPacketRun  = true;
    //접속 되어있는 클라이언트 수
    int			                mClientCnt = 0;
    std::vector<std::thread>    mIOWorkerThreads;
    std::thread                 mAccepterThread;
    std::thread                 mSendPacketThread;
    std::vector<ClientInfo>     mClientInfos;
    std::queue<UINT32>          mIdleClientIds;

    std::mutex                  mRecvPacketLock;
    std::mutex                  mSendPacketLock;
    std::queue<ClientInfo*>     mClientPoolSendingPacket;
    std::queue<std::pair<ClientInfo*, size_t>>     mClientPoolRecvedPacket;
};