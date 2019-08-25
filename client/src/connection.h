#include <stdint.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

class Connection {
public:
	Connection(const char *host, int port); // call isConnected afterwards to check if actually succeded
	~Connection();

	bool isConnected(void);

	bool sendData(const uint8_t *data, size_t len);
	bool sendStr(const char *str);
private:
	TCPsocket tcpSocket;

};
