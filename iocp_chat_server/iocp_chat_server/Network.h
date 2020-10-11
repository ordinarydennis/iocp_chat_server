#pragma once

#include "Define.h"
#include "ClientInfo.h"
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <optional>
#include <functional>

class Network
{
public:
    Network() = default;

   ~Network() { WSACleanup(); };

    Error           Init(const UINT16 port);

    void            Run();

    void            Destroy();

    void            SendData(const stPacket& packet);

    ClientInfo*     GetClientInfo(const UINT32 id);

    void            SendPacket(const stPacket& packet);

    std::function<void(stPacket)>   GetPacketSender();

    std::optional<stOverlappedEx>   GetClientRecvedPacket();

private:
    void            SetMaxThreadCount();

    Error           WinsockStartup();

    Error           CreateListenSocket();

    Error           CreateIOCP();

    void            CreateClient(const UINT32 maxClientCount);

    Error           BindandListen(const UINT16 port);

    Error           RegisterListenSocketToIOCP();

    void            SetWokerThread();

    void            WokerThread();

    void            SetSendPacketThread();

    void            SendPacketThread();

    void            ProcAcceptOperation(const stOverlappedEx* pOverlappedEx);

    void            ProcRecvOperation(const stOverlappedEx& recvOverlappedEx);

    bool            PostSendMsg(ClientInfo* pClientInfo, const char* pMsg, const UINT32 len);

    void            SetAccepterThread();

    void            AccepterThread();

    void            CloseSocket(ClientInfo* pClientInfo, const bool bIsForce = false);

    bool            BindIOCompletionPort(ClientInfo* pClientInfo);

    bool            PostRecv(ClientInfo* pClientInfo);

    void            DestroyThread();

    void            AddToClientPoolRecvPacket(const stOverlappedEx& recvOverlappedEx);

private:
    UINT16                      mMaxThreadCount = 0;
    SOCKET                      mListenSocket   = INVALID_SOCKET;
    HANDLE                  	mIOCPHandle     = INVALID_HANDLE_VALUE;
    bool	                   	mIsWorkerRun    = true;
    bool	                   	mIsAccepterRun  = true;
    bool                        mSendPacketRun  = true;
    int			                mClientCnt = 0;
    std::vector<std::thread>    mIOWorkerThreads;
    std::thread                 mAccepterThread;
    std::thread                 mSendPacketThread;
    std::vector<ClientInfo>     mClientInfos;

    std::mutex                  mRecvPacketLock;
    std::queue<stOverlappedEx>     mClientPoolRecvedPacket;
};