#include "ClientInfo.h"

ClientInfo::ClientInfo(const ClientInfo& c)
{
	mId = c.mId;
	ZeroMemory(&m_stRecvOverlappedEx, sizeof(stOverlappedEx));
	ZeroMemory(&m_stSendOverlappedEx, sizeof(stOverlappedEx));
}
ClientInfo::ClientInfo(INT32 id)
	:mId(id)
{
	ZeroMemory(&m_stRecvOverlappedEx, sizeof(stOverlappedEx));
	ZeroMemory(&m_stSendOverlappedEx, sizeof(stOverlappedEx));
}
void ClientInfo::SetClientSocket(SOCKET socketClient)
{ 
	mClientSocket = socketClient;
}
void ClientInfo::SetRecvOverlappedEx(stOverlappedEx overlappedEx)
{
	m_stRecvOverlappedEx = overlappedEx;
}
void ClientInfo::SetSendOverlappedEx(const stOverlappedEx& overlappedEx)
{
	m_stSendOverlappedEx = overlappedEx;
}
bool ClientInfo::IsSending()
{
	std::lock_guard<std::mutex> guard(mSendingLock);
	return m_bSending;
}
void ClientInfo::SetSending(bool bSending)
{
	std::lock_guard<std::mutex> guard(mSendingLock);
	m_bSending = bSending;
}
void ClientInfo::SendMgs(char* mgs)
{
	stPacket p;

}
void ClientInfo::SetLastSendPacket(const stPacket& packet)
{
	mLastSendPacket = packet;
}
stPacket ClientInfo::GetRecvPacket()
{
	std::lock_guard<std::mutex> guard(mRecvPacketPoolLock);
	if (mRecvPacketPool.empty())
	{
		return stPacket();
	}
	
	stPacket p = mRecvPacketPool.front();
	mRecvPacketPool.pop();
	return p;
}
stPacket ClientInfo::GetSendPacket()
{
	std::lock_guard<std::mutex> guard(mSendPacketPoolLock);
	if (mSendPacketPool.empty())
	{
		return stPacket();
	}

	stPacket p = mSendPacketPool.front();
	mSendPacketPool.pop();
	return p;
}
void ClientInfo::AddRecvPacket(stPacket p)
{
	std::lock_guard<std::mutex> guard(mRecvPacketPoolLock);
	mRecvPacketPool.push(p);
}
void ClientInfo::AddSendPacket(stPacket p)
{
	std::lock_guard<std::mutex> guard(mSendPacketPoolLock);
	mSendPacketPool.push(p);
}
