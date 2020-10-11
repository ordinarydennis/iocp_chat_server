#pragma once

#include "Define.h"
#include "RedisPacket.h"
#include <unordered_map>
#include <functional>
#include <thread>

struct stPacket;
class ClientInfo;
class RoomManager;
class Redis;
class ChatUserManager;
class Network;

class PacketProcessor
{
public:
    PacketProcessor();

	~PacketProcessor();

	Error			Init(const UINT16 port);

    void            Run();

    void            ProcessPacket(const stOverlappedEx& recvOverlappedEx);

    void            ProcessRedisPacket(const RedisTask& task);
    
    void            SendPacket(const UINT32 from, const UINT32 to, const UINT16 packetId, const char* body, const size_t bodySize);
    
    void            ProcEcho(const stPacket& packet);
    
    void            ProcLogin(const stPacket& packet);
    
    void            ProcRoomEnter(const stPacket& packet);
    
    void            ProcRoomChat(const stPacket& packet);
    
    void            ProcRoomLeave(const stPacket& packet);

    void            RedisProcLogin(const RedisTask& task);

    void            Destroy();

private:
    void            ReceivePacketThread();

    void            RegisterRecvProc();

private:
    using receiver = void(PacketProcessor::*)(const stPacket& p);
    std::unordered_map<PacketID, receiver>  mRecvPacketProcDict;

    using RedisReceiver = void(PacketProcessor::*)(const RedisTask& p);
    std::unordered_map<RedisTaskID, RedisReceiver>  mRecvRedisPacketProcDict;

    std::function<void(stPacket)>           mPacketSender;

    RoomManager*                            mRoomManager = nullptr;
    Redis*                                  mRedis = nullptr;
    ChatUserManager*                        mChatUserManager = nullptr;
    Network*                                mNetwork = nullptr;

    std::thread	                            mReceivePacketThread;
    bool                                    mReceivePacketRun = true;
};

