#ifndef TREMORENGINE_UDPPACKET_H
#define TREMORENGINE_UDPPACKET_H

#include <cstdint>

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

namespace TremorEngine {

	class UdpPacket {
	public:
		struct PlayerEntry {
			float x, y, z, yaw;
		};

		UdpPacket();
		UdpPacket(uint32_t id);
		~UdpPacket();

		bool initFromRecvData(const UDPpacket &rawPacket);

		bool addPlayerEntry(const PlayerEntry &playerEntry);

		uint32_t id;
		int playerCount;
		PlayerEntry players[256]; // TODO: Avoid hardcoded/magic number array size here and when accessed
	private:
	};

};

#endif
