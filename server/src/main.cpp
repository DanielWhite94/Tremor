#include <cstdarg>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

#include <engine.h>

using namespace TremorEngine;

#define serverMaxClients 32
#define serverClientTcpBufferSize 1024

struct ServerClient {
	int tcSocketSetNumber; // set to -1 if slot unused
	TCPsocket tcpSocket;
	uint8_t tcpBuffer[serverClientTcpBufferSize];
	size_t tcpBufferNext;

	UDPsocket udpSocket;
	uint32_t udpSecret;
};
ServerClient serverClients[serverMaxClients];

TCPsocket serverTcpSocket=NULL;
SDLNet_SocketSet serverSocketSet=NULL;

Map *map=NULL;

void serverInit(const char *mapFile);
void serverQuit(void);

int serverGetClientCount(void);
bool serverAcceptClient(void);
void serverRemoveClient(int id);
void serverReadClients(void);
bool serverReadClient(ServerClient &client);
bool serverSendDataToClient(ServerClient &client, const uint8_t *data, size_t len);
bool serverSendStrToClient(ServerClient &client, const char *str);

void serverLog(const char *format, ...);
void serverLogV(const char *format, va_list ap);

uint8_t serverRand8(void);
uint16_t serverRand16(void);
uint32_t serverRand32(void);

void serverSigIntHandler(int s);

int main(int argc, char **argv) {
	// Check and parse arguments
	if (argc!=2) {
		serverLog("Usage: %s mapfile\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	const char *mapFile=argv[1];

	// Initialise
	serverInit(mapFile);

	// Open server socket
	IPaddress ip;
	if(SDLNet_ResolveHost(&ip,NULL,9999)==-1) {
		serverLog("Could not resolve host: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}

	serverTcpSocket=SDLNet_TCP_Open(&ip);
	if(serverTcpSocket==NULL) {
		serverLog("Could not open TCP socket: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}

	// Allocate socket set so we can manage multiple sockets
	serverSocketSet=SDLNet_AllocSocketSet(serverMaxClients);
	if(serverSocketSet==NULL) {
		serverLog("Could not alloacte socket set: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}

	// Main loop
	while(1) {
		// Attempt to accept new client connection
		serverAcceptClient();

		// Check for socket activity
		serverReadClients();

		// Delay
		// TODO: probably want to remove this
		usleep(10000);
	}

	// Quit
	serverQuit();

	return 0;
}

void serverInit(const char *mapFile) {
	serverLog("Tremor Server\n");
	serverLog("Initialising...\n");

	// Initialise PRNG
	srand(time(NULL)); // TODO: do better

	// Mark all entries in the clients array empty
	for(size_t i=0; i<serverMaxClients; ++i)
		serverClients[i].tcSocketSetNumber=-1;

	// Register interrupt signal handler
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler=serverSigIntHandler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags=0;
	sigaction(SIGINT, &sigIntHandler, NULL);

	// Initialise SDL (for networking libraries)
	if (SDL_Init(0)<0) {
		serverLog("SDL could not initialise: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	if(SDLNet_Init()!=0) {
		serverLog("SDLNet could not initialise: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}

	serverLog("Initialised SDL\n");

	// Load map
	map=new Map(NULL, mapFile);
	if (map==NULL || !map->getHasInit()) {
		serverLog("Could not load map at: %s\n", mapFile);
		exit(EXIT_FAILURE);
	}

	serverLog("Loaded map at: %s\n", mapFile);

	// Write to log
	serverLog("Initialisation Complete\n");
}

void serverQuit(void) {
	// Write to log
	serverLog("Server quitting...\n");

	// Close all client connections
	for(size_t i=0; i<serverMaxClients; ++i) {
		// No client in this slot?
		if (serverClients[i].tcSocketSetNumber==-1)
			continue;

		// Remove from socket set and close socket
		SDLNet_TCP_DelSocket(serverSocketSet, serverClients[i].tcpSocket);
		SDLNet_TCP_Close(serverClients[i].tcpSocket);
	}

	// Free socket set
	if (serverSocketSet!=NULL)
		SDLNet_FreeSocketSet(serverSocketSet);

	// Close server socket
	if (serverTcpSocket!=NULL)
		SDLNet_TCP_Close(serverTcpSocket);

	// Quit SDL
	SDL_Quit();
}

int serverGetClientCount(void) {
	int count=0;
	for(size_t i=0; i<serverMaxClients; ++i)
		count+=(serverClients[i].tcSocketSetNumber>=0);
	return count;
}

bool serverAcceptClient(void) {
	// Too many clients already?
	if (serverGetClientCount()==serverMaxClients)
		return false;

	// Attempt to accept new socket.
	TCPsocket clientTcpSocket=SDLNet_TCP_Accept(serverTcpSocket);
	if (clientTcpSocket==NULL)
		return false;

	// Add socket to set
	int clientTcpSocketSetNumber=SDLNet_TCP_AddSocket(serverSocketSet, clientTcpSocket); // TODO: check result?
	assert(clientTcpSocketSetNumber>=0 && clientTcpSocketSetNumber<serverMaxClients);

	// Add client to array
	ServerClient &client=serverClients[clientTcpSocketSetNumber];
	assert(client.tcSocketSetNumber==-1);
	client.tcSocketSetNumber=clientTcpSocketSetNumber;
	client.tcpSocket=clientTcpSocket;
	client.tcpBufferNext=0;
	client.udpSocket=NULL;
	client.udpSecret=serverRand32();

	// Write to log
	IPaddress *remoteIp=SDLNet_TCP_GetPeerAddress(clientTcpSocket);
	uint32_t host=remoteIp->host;
	serverLog("New client: id=%i tcp addr=(%u.%u.%u.%u:%u), secret 0x%08X\n", clientTcpSocketSetNumber, host>>24, (host>>16)&255, (host>>8)&255, host&255, remoteIp->port, client.udpSecret);

	return true;
}

void serverRemoveClient(int id) {
	// Bad id?
	if (id<0 || id>=serverMaxClients)
		return;

	// No client anyway?
	ServerClient &client=serverClients[id];
	if (client.tcSocketSetNumber==-1)
		return;

	// Close socket and mark disconnected
	SDLNet_TCP_Close(client.tcpSocket);
	client.tcSocketSetNumber=-1;

	// Write to log
	serverLog("Client %i disconnected\n", id);
}

void serverReadClients(void) {
	while(SDLNet_CheckSockets(serverSocketSet, 0)>0) {
		for(size_t i=0; i<serverMaxClients; ++i) {
			// Grab client in this slot
			ServerClient &client=serverClients[i];
			if (client.tcSocketSetNumber==-1)
				continue;

			// No activity for this client?
			if (!SDLNet_SocketReady(client.tcpSocket))
				continue;

			// Buffer full - if so discard first byte (otherwise client would be stuck).
			if (client.tcpBufferNext==serverClientTcpBufferSize) {
				memmove(client.tcpBuffer, client.tcpBuffer+1, serverClientTcpBufferSize-1);
				--client.tcpBufferNext;
			}

			// Read one byte
			// TODO: SDL_net seems limited in this regard - passing maxlen>1 may block.
			if (SDLNet_TCP_Recv(client.tcpSocket, client.tcpBuffer+client.tcpBufferNext, 1)<=0) {
				serverLog("Client %i bad TCP read, disconnecting client\n", i);
				serverRemoveClient(i);
				continue;
			}
			++client.tcpBufferNext;

			// See if we have received a full command
			if (!serverReadClient(client)) {
				serverRemoveClient(i);
				continue;
			}
		}
	}
}

bool serverReadClient(ServerClient &client) {
	char responseStr[256];

	while(1) {
		// Check for a full command (terminated by either '\n' or '\r\n')
		uint8_t *newlinePtr=(uint8_t *)memchr(client.tcpBuffer, '\n', client.tcpBufferNext);
		if (newlinePtr==NULL)
			break;

		if (newlinePtr>client.tcpBuffer && *(newlinePtr-1)=='\r')
			*(--newlinePtr)='\n'; // HACK to trim '\r' - results in a 2nd empty command straight after this one

		size_t commandLen=newlinePtr-client.tcpBuffer;

		// Attempt to parse command
		// HACK: parse command as a string
		*newlinePtr='\0';
		const char *command=(const char *)client.tcpBuffer;

		if (strcmp(command, "quit")==0)
			return false;
		else if (strncmp(command, "get ", 4)==0) {
			const char *commandGetName=command+4;
			if (strcmp(commandGetName, "secret")==0) {
				sprintf(responseStr, "got secret %08X\n", client.udpSecret);
				if (!serverSendStrToClient(client, responseStr))
					return false;
			}
		}

		// Strip command from front of buffer
		memmove(client.tcpBuffer, client.tcpBuffer+commandLen+1, client.tcpBufferNext-=commandLen+1);
	}

	return true;
}

bool serverSendDataToClient(ServerClient &client, const uint8_t *data, size_t len) {
	// TODO: this is blocking, make it not so - another SDL_net limitation
	return (SDLNet_TCP_Send(client.tcpSocket, data, len)==len);
}

bool serverSendStrToClient(ServerClient &client, const char *str) {
	return serverSendDataToClient(client, (const uint8_t *)str, strlen(str));
}

void serverSigIntHandler(int s) {
	serverQuit();
	exit(EXIT_FAILURE);
}

void serverLog(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	serverLogV(format, ap);
	va_end(ap);
}

void serverLogV(const char *format, va_list ap) {
	// Print timestamp
	long long timeMs=microSecondsGet()/1000;
	printf("%02llu:%02llu:%02llu:%03llu ", (timeMs/1000/60/60), (timeMs/1000/60)%60, (timeMs/1000)%60, timeMs%1000);

	// Print message
	vprintf(format, ap);
}

uint8_t serverRand8(void) {
	return rand()&255;
}

uint16_t serverRand16(void) {
	return ((((uint16_t)serverRand8())<<8)|serverRand8());
}

uint32_t serverRand32(void) {
	return ((((uint32_t)serverRand16())<<16)|serverRand16());
}
