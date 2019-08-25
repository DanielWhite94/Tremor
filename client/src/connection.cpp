#include "connection.h"

Connection::Connection(const char *host, int port) {
	// Set tcpSocket to NULL so that if we return early in error then isConnected will correctly return false.
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
}

Connection::~Connection() {
	if (tcpSocket!=NULL)
		SDLNet_TCP_Close(tcpSocket);
}

bool Connection::isConnected(void) {
	return (tcpSocket!=NULL);
}
