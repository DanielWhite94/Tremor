#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <engine.h>

using namespace TremorEngine;

// Parameters
const int windowWidth=2*640;
const int windowHeight=2*480;

const double fps=2*30.0;

const double moveSpeed=0.07;
const double strafeSpeed=0.06;
const double turnFactor=0.0018;
const double verticalTurnFactor=0.0012;
const Object::MovementParameters playerMovementParametersStart={.jumpTime=1000000llu, .standHeight=0.5, .crouchHeight=0.3, .jumpHeight=0.3};

// Variables
SDL_Window *window;
SDL_Renderer *sdlRenderer;

Renderer *renderer=NULL;

bool showZBuffer=false;

Map *map=NULL;

Object *playerObject=NULL;

// Functions
void demoInit(const char *mapFile);
void demoQuit(void);

void demoCheckEvents(void);
void demoPhysicsTick(void);
void demoRedraw(void);

int main(int argc, char **argv) {
	// Parse arguments.
	if (argc!=2) {
		printf("Usage: %s mapfile\n", argv[0]);
		return 0;
	}

	const char *mapFile=argv[1];

	// Initialise
	demoInit(mapFile);

	// Main loop
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

	// Quit
	demoQuit();

	return EXIT_SUCCESS;
}

void demoInit(const char *mapFile) {
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

	// Set blend mode to 'blend' so that rendering objects with transparency works.
	SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND);

	// Turn on relative mouse mode to hide cursor and still generate relative changes when cursor hits the edge of the window.
	SDL_SetRelativeMouseMode(SDL_TRUE);

	// Create map
	map=new Map(sdlRenderer, mapFile);
	if (!map->getHasInit()) {
		printf("Could not load map at '%s'.\n", mapFile);
		exit(EXIT_FAILURE);
	}

	// Create ray casting renderer
	double unitBlockHeight=(512.0*windowWidth)/640.0;
	renderer=new Renderer(sdlRenderer, windowWidth, windowHeight, unitBlockHeight, &mapGetBlockInfoFunctor, map, &mapGetObjectsInRangeFunctor, map);
	renderer->setBrightnessMin(map->getBrightnessMin());
	renderer->setBrightnessMax(map->getBrightnessMax());
	renderer->setGroundColour(map->getGroundColour());
	renderer->setSkyColour(map->getSkyColour());

	// Create player object
	Camera playerCamera(map->getWidth()/2.0, map->getHeight()/2.0, playerMovementParametersStart.standHeight, 0.0);
	playerCamera.setPitchLimits(-M_PI/4.0, M_PI/4.0); // ray casting doesn't work so well beyond these limits
	playerObject=new Object(0.3, 0.6, playerCamera, playerMovementParametersStart);
}

void demoQuit(void) {
	delete playerObject;

	delete renderer;

	delete map;

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
				switch(event.key.keysym.sym) {
					case SDLK_q:
						exit(EXIT_SUCCESS);
					break;
					case SDLK_z:
						showZBuffer=!showZBuffer;
					break;
				}
			break;
			case SDL_MOUSEMOTION:
				// Horizontal motion adjusts camera camera yaw
				playerObject->turn(turnFactor*event.motion.xrel);

				// Vertical motion adjusts camera pitch
				playerObject->pitch(-verticalTurnFactor*event.motion.yrel);
			break;
		}
	}
}

void demoPhysicsTick(void) {
	// Inspect keyboard state to potentially move player/camera
	SDL_PumpEvents();
	const Uint8 *state=SDL_GetKeyboardState(NULL);

	// Running?
	double trueMoveSpeed=moveSpeed;
	double trueStrafeSpeed=strafeSpeed;
	if (state[SDL_SCANCODE_LSHIFT] && playerObject->getMovementState()!=Object::MovementState::Crouching) {
		trueMoveSpeed*=2;
		trueStrafeSpeed*=2;
	}

	// Handle WASD
	if (state[SDL_SCANCODE_A])
		playerObject->strafe(-trueStrafeSpeed);
	if (state[SDL_SCANCODE_D])
		playerObject->strafe(trueStrafeSpeed);
	if (state[SDL_SCANCODE_W])
		playerObject->move(trueMoveSpeed);
	if (state[SDL_SCANCODE_S])
		playerObject->move(-trueMoveSpeed);

	// Set crouching state and/or begin jump.
	playerObject->crouch(state[SDL_SCANCODE_LCTRL]);
	if (state[SDL_SCANCODE_SPACE])
		playerObject->jump();

	// Tick
	playerObject->tick();

	// Print position for debugging
	const Camera &camera=playerObject->getCamera();
	printf("camera (x,y,z,yaw,pitch)=(%f,%f,%f,%f,%f)\n", camera.getX(), camera.getY(), camera.getZ(), camera.getYaw(), camera.getPitch());
}

void demoRedraw(void) {
	// Perform ray casting
	renderer->render(playerObject->getCamera(), showZBuffer);
	renderer->renderTopDown(playerObject->getCamera());

	// Update the screen.
	SDL_RenderPresent(sdlRenderer);
}
