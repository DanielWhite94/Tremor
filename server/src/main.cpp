#include <cstdio>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

int main(int argc, char **argv) {
	// Init
	if (SDL_Init(0)<0) {
		printf("SDL could not initialize: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	// ...

	// Quit
	SDL_Quit();

	return 0;
}
