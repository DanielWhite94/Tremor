#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <engine.h>

#include "map.h"

using namespace TremorEngine;

// Parameters
const int windowWidth=640;
const int windowHeight=480;

const double fps=30.0;

const double moveSpeed=0.07;
const double strafeSpeed=0.06;
const double turnFactor=0.0006;
const double verticalTurnFactor=0.0004;
const MicroSeconds jumpTime=1000000llu;
double standHeight=0.5; // actual height when standing, fraction of unit block height - should be 0.5 for best graphics
double crouchHeight=0.3; // actual height when crouched, fraction of unit block height
const double jumpHeight=0.3; // max vertical displacement

Camera camera(-5.928415,10.382321,0.500000,6.261246);

// Variables
SDL_Window *window;
SDL_Renderer *sdlRenderer;

Renderer *renderer=NULL;

bool isCrouching=false;
bool isJumping=false;
MicroSeconds jumpStartTime;

// Functions
void demoInit(void);
void demoQuit(void);

void demoCheckEvents(void);
void demoPhysicsTick(void);
void demoRedraw(void);

int main(int argc, char **argv) {
	demoInit();

	while(1) {
		MicroSeconds tickStartTime=microSecondsGet();

		// Check events
		demoCheckEvents();

		// Physics tick to handle things such as player/camera movement
		demoPhysicsTick();

		// Redraw
		demoRedraw();

		// Delay for constant FPS
		MicroSeconds tickEndTime=microSecondsGet();
		double tickDeltaTime=tickEndTime-tickStartTime;
		double desiredTickDeltaTime=microSecondsPerSecond/fps;
		if (tickDeltaTime<desiredTickDeltaTime)
			microSecondsDelay(desiredTickDeltaTime-tickDeltaTime);

		double actualFps=microSecondsPerSecond/(microSecondsGet()-tickStartTime);
		if (tickDeltaTime>0.0) {
			double maxFps=microSecondsPerSecond/tickDeltaTime;
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

	// Turn on relative mouse mode to hide cursor and still generate relative changes when cursor hits the edge of the window.
	SDL_SetRelativeMouseMode(SDL_TRUE);

	// Create map
	Map *map=new Map(sdlRenderer);

	// Create ray casting renderer
	double unitBlockHeight=(512.0*windowWidth)/640.0;
	renderer=new Renderer(sdlRenderer, windowWidth, windowHeight, unitBlockHeight, &mapGetBlockInfoFunctor, map);
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
			case SDL_KEYDOWN:
				if (event.key.keysym.sym==SDLK_q)
					exit(EXIT_SUCCESS);
			break;
			case SDL_MOUSEMOTION:
				// Horizontal motion adjusts camera camera yaw
				camera.turn(turnFactor*event.motion.xrel);

				// Vertical motion adjusts camera pitch
				camera.pitch(-verticalTurnFactor*event.motion.yrel);
			break;
		}
	}
}

void demoPhysicsTick(void) {
	// Inspect keyboard state to potentially move player/camera
	SDL_PumpEvents();
	const Uint8 *state=SDL_GetKeyboardState(NULL);

	double trueMoveSpeed=moveSpeed;
	double trueStrafeSpeed=strafeSpeed;
	if (state[SDL_SCANCODE_LSHIFT] && !isCrouching) {
		trueMoveSpeed*=2;
		trueStrafeSpeed*=2;
	}

	if (state[SDL_SCANCODE_A])
		camera.strafe(-trueStrafeSpeed);
	if (state[SDL_SCANCODE_D])
		camera.strafe(trueStrafeSpeed);
	if (state[SDL_SCANCODE_W])
		camera.move(trueMoveSpeed);
	if (state[SDL_SCANCODE_S])
		camera.move(-trueMoveSpeed);

	if (state[SDL_SCANCODE_LCTRL]) {
		if (!isJumping)
			isCrouching=true;
	} else
		isCrouching=false;

	if (state[SDL_SCANCODE_SPACE] && !isJumping) {
		isJumping=true;
		jumpStartTime=microSecondsGet();
	}

	// Reset camera z value for standing, but we may update this before we return.
	camera.setZ(standHeight);

	// Crouching logic
	if (isCrouching)
		camera.setZ(crouchHeight);

	// Jumping logic
	if (isJumping) {
		MicroSeconds timeDelta=microSecondsGet()-jumpStartTime;

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
	printf("camera (x,y,z,yaw,pitch)=(%f,%f,%f,%f,%f)\n", camera.getX(), camera.getY(), camera.getZ(), camera.getYaw(), camera.getPitch());
}

void demoRedraw(void) {
	// Perform ray casting
	renderer->render(camera);
	renderer->renderTopDown(camera);

	// Update the screen.
	SDL_RenderPresent(sdlRenderer);
}
