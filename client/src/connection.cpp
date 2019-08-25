#include "connection.h"

Connection::Connection(const char *host, int port) {
	// Set fields to indicate not connected so that if we return early in error then isConnected will correctly return false.
	isConnectedFlag=false;
	tcpSocket=NULL;

	// Check port number is sensible.
	if (port<0 || port>=65536)
		return;

	// Find IP address
	IPaddress address;
	if (SDLNet_ResolveHost(&address, host, port)!=0)
		return;

	// Open TCP connection
	tcpSocket=SDLNet_TCP_Open(&address);
	if (tcpSocket==NULL)
		return;

	isConnectedFlag=true;
}

Connection::~Connection() {
	if (tcpSocket!=NULL)
		SDLNet_TCP_Close(tcpSocket);
}

bool Connection::isConnected(void) {
	return isConnectedFlag;
}

bool Connection::sendData(const uint8_t *data, size_t len) {
	// TODO: this is blocking, make it not so (seems like an SDL_net limitation).
	return (SDLNet_TCP_Send(tcpSocket, data, len)==len);
}

bool Connection::sendStr(const char *str) {
	return sendData((const uint8_t *)str, strlen(str));
}
