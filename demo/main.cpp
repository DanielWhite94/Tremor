#include <cstdio>
#include <cstdlib>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <SDL2/SDL.h>

#include "../engine/camera.h"
#include "../engine/renderer.h"

using namespace RayCast;

const unsigned long long microsPerS=1000000llu;

// Parameters
const int windowWidth=1280;
const int windowHeight=960;

const double fps=120.0;

const double moveSpeed=0.025;
const double turnSpeed=M_PI/144.0;

Camera camera(0.77,15.99,5.323);

#define MapW 16
#define MapH 16
#define _ {.height=0.0}
#define W {.height=1.0}
#define D {.height=2.0}
#define T {.height=3.0}
#define H {.height=9.0}
const Renderer::BlockInfo map[MapH][MapW]={
	{_,W,W,W,_,_,_,_,_,_,_,_,_,_,_,_},
	{_,W,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
	{_,W,_,W,_,_,_,_,_,_,_,_,_,_,_,_},
	{_,W,_,W,_,_,_,_,_,_,_,_,_,_,_,_},
	{_,_,_,W,_,_,_,_,_,_,_,_,_,_,_,_},
	{_,W,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
	{_,W,_,W,W,W,W,W,W,W,W,_,_,_,_,_},
	{_,W,_,W,W,D,D,D,D,D,W,_,_,_,_,_},
	{_,_,_,_,W,D,T,T,T,D,W,_,_,_,_,_},
	{_,_,_,_,W,D,T,H,T,D,W,_,_,_,_,_},
	{_,_,_,_,W,D,T,T,T,D,W,_,_,_,_,_},
	{_,_,_,_,W,D,D,D,D,D,W,_,_,_,_,_},
	{_,_,_,_,W,W,W,W,W,W,W,_,_,_,_,_},
	{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
	{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
	{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
};
#undef _
#undef W
#undef D
#undef T
#undef H

// Variables
SDL_Window *window;
SDL_Renderer *sdlRenderer;

Renderer *renderer=NULL;

// Functions
void demoInit(void);
void demoQuit(void);

void demoCheckEvents(void);
void demoPhysicsTick(void);
void demoRedraw(void);

bool demoGetBlockInfoFunctor(int mapX, int mapY, Renderer::BlockInfo *info);

unsigned long long demoGetTimeMicro(void);
void demoDelayMicro(unsigned long long micros);

int main(int argc, char **argv) {
	demoInit();

	while(1) {
		unsigned long long tickStartTime=demoGetTimeMicro();

		// Check events
		demoCheckEvents();

		// Physics tick to handler things such as player/camera movement
		demoPhysicsTick();

		// Redraw
		demoRedraw();

		// Delay for constant FPS
		unsigned long long tickEndTime=demoGetTimeMicro();
		double tickDeltaTime=tickEndTime-tickStartTime;
		double desiredTickDeltaTime=microsPerS/fps;
		if (tickDeltaTime<desiredTickDeltaTime)
			demoDelayMicro(desiredTickDeltaTime-tickDeltaTime);

		double actualFps=microsPerS/(demoGetTimeMicro()-tickStartTime);
		if (tickDeltaTime>0.0) {
			double maxFps=microsPerS/tickDeltaTime;
			printf("fps %.1f (max %.1f)\n", actualFps, maxFps);
		} else
			printf("fps %.1f (max inf)\n", actualFps);
	}

	demoQuit();

	return EXIT_SUCCESS;
}

void demoInit(void) {
	// Initialse SDL and create window+renderer
	// TODO: Throw exceptions instead?
	if(SDL_Init(SDL_INIT_VIDEO)<0) {
		printf("SDL could not initialize: %s\n", SDL_GetError());
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

	// Create ray casting renderer
	renderer=new Renderer(sdlRenderer, windowWidth, windowHeight, &demoGetBlockInfoFunctor);
}

void demoQuit(void) {
	delete renderer;

	SDL_DestroyRenderer(sdlRenderer);
	sdlRenderer=NULL;
	SDL_DestroyWindow(window);
	window=NULL;

	SDL_Quit();
}

void demoCheckEvents(void) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT:
				exit(EXIT_SUCCESS);
			break;
		}
	}
}

void demoPhysicsTick(void) {
	// Inspect keyboard state to potentially move player/camera
	SDL_PumpEvents();
	const Uint8 *state=SDL_GetKeyboardState(NULL);
	if (state[SDL_SCANCODE_LEFT])
		camera.turn(-turnSpeed);
	if (state[SDL_SCANCODE_RIGHT])
		camera.turn(turnSpeed);
	if (state[SDL_SCANCODE_UP])
		camera.move(moveSpeed);
	if (state[SDL_SCANCODE_DOWN])
		camera.move(-moveSpeed);
}

void demoRedraw(void) {
	// Perform ray casting
	renderer->render(camera);
	//renderer->renderTopDown(camera);

	// Update the screen.
	SDL_RenderPresent(sdlRenderer);
}

bool demoGetBlockInfoFunctor(int mapX, int mapY, Renderer::BlockInfo *info) {
	// Outside of pre-defined region?
	if (mapX<0 || mapX>=MapW || mapY<0 || mapY>=MapH) {
		// Create a grid of pillars in half the landscape at least to show distance
		if (!((mapX%5==0) && (mapY%5==0) && mapX>0))
			return false;
		info->height=6.0;
		return true;
	}

	// Inside predefined map region - so use array.
	if (map[mapY][mapX].height<0.0001)
		return false;

	*info=map[mapY][mapX];
	return true;
}

unsigned long long demoGetTimeMicro(void) {
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC_RAW, &tp); // TODO: Check return.
	return ((unsigned long long)tp.tv_sec)*microsPerS+tp.tv_nsec/1000;
}

void demoDelayMicro(unsigned long long micros) {
	struct timespec tp;
	tp.tv_sec=micros/microsPerS;
	tp.tv_nsec=(micros%microsPerS)*1000;
	nanosleep(&tp, NULL);
}
