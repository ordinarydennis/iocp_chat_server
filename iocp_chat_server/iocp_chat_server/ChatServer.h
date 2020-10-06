#pragma once

#include "Define.h"
#include "Network.h"
#include "Redis.h"
#include "RoomManager.h"
#include "ChatUserManager.h"

#include <vector>
#include <thread>
#include <memory>
#include <unordered_map>

class ChatServer
{
public:
    ChatServer() = default;
    ~ChatServer();

    Error Init();
    void Run();
    void Destroy();

private:
    void SetRedisResponseThread();
    void RedisResponseThread();
    void SetReceivePacketThread();
    void ReceivePacketThread();
    void Waiting();
    void SendPacket(UINT32 from, UINT32 to, UINT16 packetId, char* body, size_t bodySize);

    void RegisterRecvProc();
    void ProcessPacket(std::pair<ClientInfo*, size_t> packet);
    void ProcessRedisPacket(RedisTask task);
    void ProcEcho(stPacket packet);
    void ProcLogin(stPacket packet);
    void ProcRoomEnter(stPacket packet);
    void ProcRoomChat(stPacket packet);
    void ProcRoomLeave(stPacket packet);


private:
    std::thread	                mRedisResponseThread;
    std::thread	                mReceivePacketThread;
    bool                        mRedisResponseRun = true;
    bool                        mReceivePacketRun = true;
    
    using receiver = void(ChatServer::*)(stPacket p);
    std::unordered_map<PacketID, receiver> mRecvPacketProcDict;

    using redis_receiver = void(Redis::*)(const RedisTask& task);
    std::unordered_map<REDIS_TASK_ID, redis_receiver> mRecvRedisPacketProcDict;

    std::unique_ptr<Network>    mNetwork;
    std::unique_ptr<Redis>      mRedis;
    RoomManager                 mRoomManager;
    ChatUserManager             mChatUserManager;
};
