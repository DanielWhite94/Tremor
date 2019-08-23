#include <cstdarg>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <signal.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

#include <engine.h>

using namespace TremorEngine;

#define serverMaxClients 32

struct ServerClient {
	int socketSetNumber; // set to -1 if slot unused
	TCPsocket tcpSocket;
};
ServerClient serverClients[serverMaxClients];

TCPsocket serverTcpSocket=NULL;
SDLNet_SocketSet serverSocketSet=NULL;

Map *map=NULL;

void serverInit(const char *mapFile);
void serverQuit(void);

int serverGetClientCount(void);
bool serverAcceptClient(void);

void serverLog(const char *format, ...);
void serverLogV(const char *format, va_list ap);

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

	// Mark all entries in the clients array empty
	for(size_t i=0; i<serverMaxClients; ++i)
		serverClients[i].socketSetNumber=-1;

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
		if (serverClients[i].socketSetNumber==-1)
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
		count+=(serverClients[i].socketSetNumber>=0);
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
	int clientSocketSetNumber=SDLNet_TCP_AddSocket(serverSocketSet, clientTcpSocket); // TODO: check result?
	assert(clientSocketSetNumber>=0 && clientSocketSetNumber<serverMaxClients);

	// Add client to array
	assert(serverClients[clientSocketSetNumber].socketSetNumber==-1);
	serverClients[clientSocketSetNumber].socketSetNumber=clientSocketSetNumber;
	serverClients[clientSocketSetNumber].tcpSocket=clientTcpSocket;

	// Write to log
	IPaddress *remoteIp=SDLNet_TCP_GetPeerAddress(clientTcpSocket);
	uint32_t host=remoteIp->host;
	serverLog("New client %i (%u.%u.%u.%u:%u)\n", clientSocketSetNumber, host>>24, (host>>16)&255, (host>>8)&255, host&255, remoteIp->port);

	return true;
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
	// TODO: Add timestamps and any other data
	vprintf(format, ap);
}
