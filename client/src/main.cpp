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

// Functions
void clientInit(const char *serverHost, int serverPort);
void clientQuit(void);

void clientCheckSdlEvents(void);

void clientCheckConnectionEvents(void);

int main(int argc, char **argv) {
	// Parse arguments.
	if (argc!=3) {
		printf("Usage: %s host port\n", argv[0]);
		return 0;
	}

	const char *serverHost=argv[1];
	int serverPort=atoi(argv[2]);

	// Initialise
	clientInit(serverHost, serverPort);

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

void clientInit(const char *serverHost, int serverPort) {
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
	serverConnection=new Connection(serverHost, serverPort);
	if (!serverConnection->isConnected()) {
		printf("Could not connect to server at: %s:%i\n", serverHost, serverPort);
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

}
