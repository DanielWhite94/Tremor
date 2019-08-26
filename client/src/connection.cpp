#include <cstring>

#include "connection.h"

Connection::Connection(const char *host, int port) {
	// Set fields to indicate not connected so that if we return early in error then isConnected will correctly return false.
	isConnectedFlag=false;
	tcpSocket=NULL;
	udpSocket=NULL;
	socketSet=NULL;
	tcpBufferNext=0;

	// Check port number is sensible.
	if (port<0 || port>=65536)
		return;

	// Create socket set
	socketSet=SDLNet_AllocSocketSet(2);
	if (socketSet==NULL)
		return;

	// Find IP address
	IPaddress address;
	if (SDLNet_ResolveHost(&address, host, port)!=0)
		return;

	// Open TCP connection and add to socket set
	tcpSocket=SDLNet_TCP_Open(&address);
	if (tcpSocket==NULL)
		return;

	if (SDLNet_TCP_AddSocket(socketSet, tcpSocket)==-1)
		return;

	isConnectedFlag=true;
}

Connection::~Connection() {
	// Close udp socket
	if (udpSocket!=NULL) {
		if (socketSet!=NULL)
			SDLNet_UDP_DelSocket(socketSet, udpSocket);
		SDLNet_UDP_Close(udpSocket);
	}

	// Close tcp socket
	if (tcpSocket!=NULL) {
		if (socketSet!=NULL)
			SDLNet_TCP_DelSocket(socketSet, tcpSocket);
		SDLNet_TCP_Close(tcpSocket);
	}

	// Free socket set
	if (socketSet!=NULL)
		SDLNet_FreeSocketSet(socketSet);
}

bool Connection::isConnected(void) {
	return isConnectedFlag;
}

bool Connection::connectUdp(const char *host, int port, uint32_t secret) {
	// Already connected?
	if (udpSocket!=NULL)
		return false;

	// Resolve IP address
	if (SDLNet_ResolveHost(&udpAddress, host, port)!=0)
		return false;

	// Open UDP connection and add to socket set
	udpSocket=SDLNet_UDP_Open(0);
	if (udpSocket==NULL)
		return false;

	if (SDLNet_UDP_AddSocket(socketSet, udpSocket)==-1) {
		SDLNet_UDP_Close(udpSocket);
		udpSocket=NULL;
		return false;
	}

	// Send secret back to server so it can associate our two connections
	char buffer[16];
	sprintf(buffer, "%08X", secret);
	if (!udpSendStr(buffer))
		return false;

	return true;
}

bool Connection::readLine(char *buffer) {
	// Check for new data
	bool change=false;
	while(SDLNet_CheckSockets(socketSet, 0)>0) {
		// No TCP activity?
		if (!SDLNet_SocketReady(tcpSocket))
			break;

		// Is buffer full? If so discard first byte (otherwise would be stuck).
		if (tcpBufferNext==tcpBufferSize) {
			memmove(tcpBuffer, tcpBuffer+1, tcpBufferSize-1);
			--tcpBufferNext;
		}

		// Read one byte
		// TODO: SDL_net seems limited in this regard - passing maxlen>1 may block.
		if (SDLNet_TCP_Recv(tcpSocket, tcpBuffer+tcpBufferNext, 1)<=0)
			break;
		++tcpBufferNext;
		change=true;
	}
	if (!change)
		return false;

	// Check for a full command (terminated by '\n')
	uint8_t *newlinePtr=(uint8_t *)memchr(tcpBuffer, '\n', tcpBufferNext);
	if (newlinePtr==NULL)
		return false;

	size_t commandLen=newlinePtr-tcpBuffer;

	// Copy data as a string into caller's buffer (HACK)
	*newlinePtr='\0';
	const char *command=(const char *)tcpBuffer;
	strcpy(buffer, command);

	// Strip command from front of buffer
	memmove(tcpBuffer, tcpBuffer+commandLen+1, tcpBufferNext-=commandLen+1);

	return true;
}

bool Connection::sendData(const uint8_t *data, size_t len) {
	// TODO: this is blocking, make it not so (seems like an SDL_net limitation).
	return (SDLNet_TCP_Send(tcpSocket, data, len)==len);
}

bool Connection::sendStr(const char *str) {
	return sendData((const uint8_t *)str, strlen(str));
}

bool Connection::udpSendData(const uint8_t *data, size_t len) {
	if (udpSocket==NULL)
		return false;

	// Form packet
	UDPpacket packet;
	packet.data=(uint8_t *)data;
	packet.len=len;
	packet.maxlen=len;
	packet.address=udpAddress;

	// Attempt to send
	return (SDLNet_UDP_Send(udpSocket, -1, &packet)==1);
}

bool Connection::udpSendStr(const char *str) {
	return udpSendData((const uint8_t *)str, strlen(str));
}
