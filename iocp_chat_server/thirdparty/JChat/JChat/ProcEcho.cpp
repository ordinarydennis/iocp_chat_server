#include "PacketProcessor.h"

namespace JChat
{
	void PacketProcessor::ProcEcho(const JCommon::stPacket& packet)
	{
		JCommon::stPacket resPacket = packet;
		resPacket.mClientTo = packet.mClientFrom;
		mPacketSender(resPacket);
	}
}

