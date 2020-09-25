#pragma once

#include <vector>
#include <thread>
#include <memory>
#include <unordered_map>
#include "Define.h"
#include "Network.h"
#include "Redis.h"

class ChatServer
{
public:
    ChatServer() = default;
    ~ChatServer() { WSACleanup(); };
    //ChatServer(const ChatServer&) = delete;
    //ChatServer(ChatServer&&) = delete;
    //ChatServer& operator=(const ChatServer&) = delete;
    //ChatServer& operator=(ChatServer&&) = delete;

    void Init();
    void Run();
    void Destroy();

private:
    void SetRedisResponseThread();
    void RedisResponseThread();
    void SetReceivePacketThread();
    void ReceivePacketThread();
    void SetSendPacketThread();
    void SendPacketThread();
    void Waiting();

    //프로시저
    void RegisterRecvProc();
    void ProcessPacket(stPacket packet);
    void ProcEcho(stPacket packet);
    void ProcLogin(stPacket packet);
    void ProcRoonEnter(stPacket packet);

private:
    std::thread	                mRedisResponseThread;
    std::thread	                mReceivePacketThread;
    std::vector<std::thread>    mSendPacketThreads;
    bool                        mRedisResponseRun = true;
    bool                        mReceivePacketRun = true;
    bool                        mSendPacketRun = true;
    std::unique_ptr<Network>    mNetwork;

    using receiver = void(ChatServer::*)(stPacket p);
    std::unordered_map<PacketID, receiver> mRecvPacketProcDict;

    std::unique_ptr<Redis>    mRedis;
};
