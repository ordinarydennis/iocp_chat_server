#pragma once

#include "Define.h"

class PacketProcessor;

class ChatServer
{
public:
    ChatServer();

    ~ChatServer();

	Error Init(const UINT16 port);

    void Run();

    void Destroy();

private:
    void Waiting();

private:
    PacketProcessor*            mPacketProcessor = nullptr;

};
