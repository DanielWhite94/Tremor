#include <cassert>

#include "udppacket.h"

namespace TremorEngine {
	UdpPacket::UdpPacket() {
		id=0;
		playerCount=0;
	}

	UdpPacket::UdpPacket(uint32_t id): id(id) {
		playerCount=0;
	}

	UdpPacket::~UdpPacket() {
	}

	bool UdpPacket::initFromRecvData(const UDPpacket &rawPacket) {
		// We need at least id+playerCount
		if (rawPacket.len<4+1)
			return false;

		// Parse data
		const uint8_t *dataPtr=rawPacket.data;

		// Read id
		id=0;
		id|=(((uint32_t)(*dataPtr++))<<24);
		id|=(((uint32_t)(*dataPtr++))<<16);
		id|=(((uint32_t)(*dataPtr++))<<8);
		id|=(((uint32_t)(*dataPtr++))<<0);

		// Read player count
		playerCount=*dataPtr++;

		// Read player data
		// TODO: Avoid reading past rawPacket.len (especilly rawPacket.maxlen)
		for(unsigned i=0; i<playerCount; ++i) {
			PlayerEntry entry;
			memcpy((void *)&entry.x, (void *)dataPtr, sizeof(float));
			dataPtr+=sizeof(float);
			memcpy((void *)&entry.y, (void *)dataPtr, sizeof(float));
			dataPtr+=sizeof(float);
			memcpy((void *)&entry.z, (void *)dataPtr, sizeof(float));
			dataPtr+=sizeof(float);
			memcpy((void *)&entry.yaw, (void *)dataPtr, sizeof(float));
			dataPtr+=sizeof(float);

			addPlayerEntry(entry);
		}

		return true;
	}

	bool UdpPacket::addPlayerEntry(const PlayerEntry &playerEntry) {
		if (playerCount==256)
			return false;

		players[playerCount++]=playerEntry;

		return true;
	}

};
