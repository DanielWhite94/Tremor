#include <stdint.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

class Connection {
public:
	Connection(const char *host, int port); // call isConnected afterwards to check if actually succeded
	~Connection();

	bool isConnected(void);

	bool connectUdp(const char *host, int port, uint32_t secret);

	bool readLine(char *buffer); // TODO: add buffer size to avoid overflows

	bool sendData(const uint8_t *data, size_t len);
	bool sendStr(const char *str);
private:
	static const size_t tcpBufferSize=1024;

	bool isConnectedFlag;

	TCPsocket tcpSocket;
	SDLNet_SocketSet socketSet;

	uint8_t tcpBuffer[tcpBufferSize];
	size_t tcpBufferNext;

	UDPsocket udpSocket;
	IPaddress udpAddress;

	bool udpSendData(const uint8_t *data, size_t len);
	bool udpSendStr(const char *str);
};
