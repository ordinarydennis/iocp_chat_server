#include "ChatServer.h"
#include "Network.h"
#include "ServerConfig.h"

void ChatServer::Init()
{
	//네트워크 초기화
	NetworkInstance.Init();

	
}

void ChatServer::Run()
{
	NetworkInstance.Run();
	//네트워크의 패킷 버퍼에서 패킷을 가져와서
	//헤더 정보를 보고 분기한다.
	

	//쓰레드로 따로 뺴기
	while (1)
	{
		if (NetworkInstance.IsPacketPoolEmpty())
			continue;

		stPacket p = NetworkInstance.GetPackget();

		switch (p.mHeader.mPacket_id)
		{
		case 1:
			NetworkInstance.SendData(p.mClientId, p.mBody, strlen(p.mBody));
			break;
			
		}
	}
}
void ChatServer::Destroy()
{
	NetworkInstance.Destroy();
}