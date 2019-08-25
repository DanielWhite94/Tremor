#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

class Connection {
public:
	Connection(const char *host, int port); // call isConnected afterwards to check if actually succeded
	~Connection();

	bool isConnected(void);

private:
	TCPsocket tcpSocket;

};
