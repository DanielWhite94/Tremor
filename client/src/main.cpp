#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <engine.h>

#include "connection.h"

using namespace TremorEngine;

// Parameters
const int windowWidth=640;
const int windowHeight=480;

const char *mapFileDir="maps";

// Variables
SDL_Window *window;
SDL_Renderer *sdlRenderer;

Connection *serverConnection=NULL;

MicroSeconds mapFileRequestInterval=5*microSecondsPerSecond;
MicroSeconds mapFileLastRequestTime=0;
char *mapFile=NULL;

MicroSeconds secretRequestInterval=5*microSecondsPerSecond;
MicroSeconds secretLastRequestTime=0;
uint32_t secret;
bool haveSecret=false;

MicroSeconds udpPortRequestInterval=5*microSecondsPerSecond;
MicroSeconds udpPortLastRequestTime=0;

MicroSeconds udpConnectionRequestInterval=5*microSecondsPerSecond;
MicroSeconds udpConnectionLastRequestTime=0;

const char *serverHost=NULL;
int serverTcpPort=-1;
int serverUdpPort=-1;

// Functions
void clientInit(void);
void clientQuit(void);

void clientCheckSdlEvents(void);

void clientCheckConnectionEvents(void);

int main(int argc, char **argv) {
	// Parse arguments.
	if (argc!=3) {
		printf("Usage: %s host port\n", argv[0]);
		return 0;
	}

	serverHost=argv[1];
	serverTcpPort=atoi(argv[2]);

	// Initialise
	clientInit();

	// Main loop
	while(1) {
		// Check SDL events
		clientCheckSdlEvents();

		// Check connection events
		clientCheckConnectionEvents();
	}

	// Quit
	clientQuit();

	return EXIT_SUCCESS;
}

void clientInit() {
	// Initialse SDL and create window+renderer
	// TODO: Throw exceptions instead?
	if(SDL_Init(SDL_INIT_VIDEO)<0) {
		printf("SDL could not initialize: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	if(SDLNet_Init()!=0) {
		printf("SDLNet could not initialise: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}

	window=SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
	if(window==NULL) {
		printf("Window could not be created: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	sdlRenderer=SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if(sdlRenderer==NULL) {
		printf("Renderer could not be created: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	// Set blend mode to 'blend' so that rendering objects with transparency works.
	SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND);

	// Turn on relative mouse mode to hide cursor and still generate relative changes when cursor hits the edge of the window.
	SDL_SetRelativeMouseMode(SDL_TRUE);

	// Attempt to connect to server
	serverConnection=new Connection(serverHost, serverTcpPort);
	if (!serverConnection->isConnected()) {
		printf("Could not connect to server at: %s:%i\n", serverHost, serverTcpPort);
		exit(EXIT_FAILURE);
	}
}

void clientQuit(void) {
	delete serverConnection;

	SDL_DestroyRenderer(sdlRenderer);
	sdlRenderer=NULL;
	SDL_DestroyWindow(window);
	window=NULL;

	SDL_Quit();

	free(mapFile);
}

void clientCheckSdlEvents(void) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT:
				exit(EXIT_SUCCESS);
			break;
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym) {
					case SDLK_q:
						exit(EXIT_SUCCESS);
					break;
				}
			break;
		}
	}
}

void clientCheckConnectionEvents(void) {
	// Check if any TCP data has arrived.
	char line[1024];
	while(serverConnection->readLine(line)) {
		if (strncmp(line, "got ", 4)==0) {
			const char *gotName=line+4;
			if (strncmp(gotName, "map ", 4)==0) {
				// Grab map base name
				const char *mapFileBasename=gotName+4;

				// Create full map file path
				mapFile=(char *)malloc(strlen(mapFileDir)+1+strlen(mapFileBasename)+1); // TODO: check return
				sprintf(mapFile, "%s/%s", mapFileDir, mapFileBasename);

				// Print info
				printf("Server map file is '%s'\n", mapFile);
			} else if (strncmp(gotName, "secret ", 7)==0) {
				// Grab secret hex string
				const char *secretHex=gotName+7;

				// Convert hex to integer
				if (sscanf(secretHex, "%08X", &secret)==1) {
					// Set flag to indicate we have received this value
					haveSecret=true;

					// Print info
					printf("Received secret %08X from server\n", secret);
				}
			} else if (strncmp(gotName, "udpport ", 8)==0) {
				// Grab secret hex string
				const char *udpPortStr=gotName+8;

				// Convert to integer
				serverUdpPort=atoi(udpPortStr);

				// Print info
				printf("Received UDP port %u from server\n", serverUdpPort);
			}
		}
	}

	// Do we still require a map file?
	if (mapFile==NULL && microSecondsGet()-mapFileLastRequestTime>=mapFileRequestInterval) {
		// Send request
		serverConnection->sendStr("get map\n");

		// Update last request time
		mapFileLastRequestTime=microSecondsGet();

		// Print info
		printf("Requesting server map file...\n");
	}

	// Do we still require 'secret'?
	if (!haveSecret && microSecondsGet()-secretLastRequestTime>=secretRequestInterval) {
		// Send request
		serverConnection->sendStr("get secret\n");

		// Update last request time
		secretLastRequestTime=microSecondsGet();

		// Print info
		printf("Requesting 'secret' from server...\n");
	}

	// Do we still require udp port?
	if (serverUdpPort==-1 && microSecondsGet()-udpPortLastRequestTime>=udpPortRequestInterval) {
		// Send request
		serverConnection->sendStr("get udpport\n");

		// Update last request time
		udpPortLastRequestTime=microSecondsGet();

		// Print info
		printf("Requesting UDP port from server...\n");
	}

	// Do we need to open a UDP connection?
	if (!serverConnection->isConnectedUdp() && haveSecret && serverUdpPort!=-1 && microSecondsGet()-udpConnectionLastRequestTime>=udpConnectionRequestInterval) {
		// Print info
		printf("Attempting to establish UDP connection with %s:%u...\n", serverHost, serverUdpPort);

		// Attempt to connect
		if (serverConnection->connectUdp(serverHost, serverUdpPort, secret))
			printf("UDP connection established\n");
		else
			printf("UDP connection failed\n");

		// Update request logic fields
		udpConnectionLastRequestTime=microSecondsGet();
	}
}
