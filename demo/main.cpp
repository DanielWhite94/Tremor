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
const int windowWidth=640;
const int windowHeight=480;

const double fps=30.0;

const double moveSpeed=0.07;
const double turnSpeed=M_PI/100.0;
const double verticalSpeed=0.02;
const unsigned long long jumpTime=1000000llu;
double standHeight=0.5; // actual height when standing, fraction of unit block height - should be 0.5 for best graphics
double crouchHeight=0.3; // actual height when crouched, fraction of unit block height
const double jumpHeight=0.3; // max vertical displacement

Camera camera(-5.928415,10.382321,0.500000,6.261246);

#define MapW 16
#define MapH 16
#define _ {.height=0.0}
#define w {.height=1.0, .colour={.r=128, .g=128, .b=128}}
#define W {.height=0.7, .colour={.r=235, .g=158, .b=52}}
#define D {.height=1.4, .colour={.r=235, .g=170, .b=52}}
#define T {.height=2.1, .colour={.r=235, .g=200, .b=52}}
#define H {.height=2.8, .colour={.r=235, .g=232, .b=52}}
#define B {.height=1.5, .colour={.r=235, .g=50, .b=52}}
#define s1 {.height=0.1, .colour={.r=177, .g=3, .b=252}}
#define s2 {.height=0.2, .colour={.r=177, .g=3, .b=252}}
#define s3 {.height=0.3, .colour={.r=177, .g=3, .b=252}}
#define s4 {.height=0.4, .colour={.r=177, .g=3, .b=252}}
#define s5 {.height=0.5, .colour={.r=177, .g=3, .b=252}}
#define s6 {.height=0.6, .colour={.r=177, .g=3, .b=252}}
#define s7 {.height=0.7, .colour={.r=177, .g=3, .b=252}}
#define s8 {.height=0.8, .colour={.r=177, .g=3, .b=252}}
#define s9 {.height=0.9, .colour={.r=177, .g=3, .b=252}}
const Renderer::BlockInfo map[MapH][MapW]={
	{ W, W, W, _, _, _, _, _, _, _, _, _, _, _, _, _},
	{ _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _},
	{ w, w, w, _, w, _, B, B, B, _, B, B, B, _, B, B},
	{ w, _, _, _, w, _, _, _, _, _, _, _, _, _, _, B},
	{ w, _, w, _, _, _, _, _, _, _, _, _, _, _, _, _},
	{ w, _, w, _, w, w, w, _, _, _, _, _, _, _, _, B},
	{ _, _, w, _, w, _, _, _, _, _, _, _, _, _, _, B},
	{ w, _, _, _, _, _, _, _, _, _, _, _, _, _, _, B},
	{ w, _, w, _, W, W, W, W, W, W, W, _, _, _, _, _},
	{ w, _, w, _, W, D, D, D, D, D, W, _, _, _, _, B},
	{ _, _, _, _, W, D, T, T, T, D, W, _, _, _, _, B},
	{ _, _, _, _, W, D, T, H, T, D, W, _, _, _, _, B},
	{s3,s2,s1, _, W, D, T, T, T, D, W, _, _, _, _, _},
	{s4,s5,s6, _, W, D, D, D, D, D, W, _, _, _, _, B},
	{s9,s8,s7, _, W, W, W, W, W, W, W, _, _, _, _, B},
	{ _, _, _, _, _, _, _, _, _, _, _, B, B, B, _, B},
};
#undef _
#undef w
#undef D
#undef T
#undef H
#undef s1
#undef s2
#undef s3
#undef s4
#undef s5
#undef s6
#undef s7
#undef s8
#undef s9

// Variables
SDL_Window *window;
SDL_Renderer *sdlRenderer;

Renderer *renderer=NULL;

bool isCrouching=false;
bool isJumping=false;
unsigned long long jumpStartTime;

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

	double trueMoveSpeed=moveSpeed;
	if (state[SDL_SCANCODE_LSHIFT] && !isCrouching)
		trueMoveSpeed*=2;

	if (state[SDL_SCANCODE_A])
		camera.turn(-turnSpeed);
	if (state[SDL_SCANCODE_D])
		camera.turn(turnSpeed);
	if (state[SDL_SCANCODE_W])
		camera.move(trueMoveSpeed);
	if (state[SDL_SCANCODE_S])
		camera.move(-trueMoveSpeed);
	if (state[SDL_SCANCODE_Q]) {
		standHeight-=verticalSpeed;
		crouchHeight-=verticalSpeed;
	}
	if (state[SDL_SCANCODE_E]) {
		standHeight+=verticalSpeed;
		crouchHeight+=verticalSpeed;
	}

	if (state[SDL_SCANCODE_LCTRL]) {
		if (!isJumping)
			isCrouching=true;
	} else
		isCrouching=false;

	if (state[SDL_SCANCODE_SPACE] && !isJumping) {
		isJumping=true;
		jumpStartTime=demoGetTimeMicro();
	}

	// Reset camera z value for standing, but we may update this before we return.
	camera.setZ(standHeight);

	// Crouching logic
	if (isCrouching)
		camera.setZ(crouchHeight);

	// Jumping logic
	if (isJumping) {
		unsigned long long timeDelta=demoGetTimeMicro()-jumpStartTime;

		// Finished jumping?
		if (timeDelta>=jumpTime)
			isJumping=false;
		else {
			// Otherwise update camera Z value based on how far through jump we are
			double jumpTimeFraction=((double)timeDelta)/jumpTime;
			double currJumpHeight=sin(jumpTimeFraction*M_PI)*jumpHeight;
			camera.setZ(standHeight+currJumpHeight);
		}
	}

	// Print position for debugging
	printf("camera (x,y,z,angle)=(%f,%f,%f,%f)\n", camera.getX(), camera.getY(), camera.getZ(), camera.getAngle());
}

void demoRedraw(void) {
	// Perform ray casting
	renderer->render(camera);
	renderer->renderTopDown(camera);

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
		info->colour.r=64;
		info->colour.g=64;
		info->colour.b=64;
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
