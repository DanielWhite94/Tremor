#include <cmath>

#include "object.h"
#include "util.h"

namespace TremorEngine {
	Object::Object(double width, double height, const Camera &camera, const MovementParameters &movementParameters): width(width), height(height), camera(camera) {
		movementData.parameters=movementParameters;
		movementData.state=MovementState::Standard;
		textures=new std::vector<SDL_Texture *>;
	}

	Object::~Object() {
		delete textures;
	}

	void Object::addTexture(SDL_Texture *texture) {
		textures->push_back(texture);
	}

	double Object::getWidth(void) const {
		return width;
	}

	double Object::getHeight(void) const {
		return height;
	}

	const Camera &Object::getCamera(void) const {
		return camera;
	}

	Object::MovementState Object::getMovementState(void) const {
		return movementData.state;
	}

	int Object::getTextureCount(void) const {
		return textures->size();
	}

	SDL_Texture *Object::getTextureN(int n) const {
		if (n<0 || n>=getTextureCount())
			return NULL;

		return (*textures)[n];
	}

	SDL_Texture *Object::getTextureAngle(double angle) const {
		// TODO: this seems convoluted and can no doubt be improved
		double fraction=angleNormalise(angle)/(2.0*M_PI)-0.25;
		if (fraction<0)
			fraction+=1.0;
		int n=floor(getTextureCount()*fraction+0.5); // +0.5 is to round to nearest rather than always down
		if (n>=getTextureCount())
			n-=getTextureCount();
		return getTextureN(n);
	}

	void Object::move(double delta) {
		camera.move(delta);
	}

	void Object::strafe(double delta) {
		camera.strafe(delta);
	}

	void Object::turn(double delta) {
		camera.turn(delta);
	}

	void Object::pitch(double delta) {
		camera.pitch(delta);
	}

	bool Object::crouch(bool value) {
		if (value) {
			// Cannot crouch if already jumping
			if (getMovementState()==MovementState::Jumping)
				return false;

			// Enter/remain in crouched state
			movementData.state=MovementState::Crouching;
		} else {
			// Not already crouched?
			if (getMovementState()!=MovementState::Crouching)
				return false;

			// Leave crouched state
			movementData.state=MovementState::Standard;
		}

		return true;
	}

	bool Object::jump(void) {
		// Already mid-jump?
		if (getMovementState()==MovementState::Jumping)
			return false;

		// Begin jump
		movementData.state=MovementState::Jumping;
		movementData.jumping.startTime=microSecondsGet();

		return true;
	}

	void Object::tick(void) {
		// Reset camera z value for standing, but we may update this again before we return.
		camera.setZ(movementData.parameters.standHeight);

		// Crouching logic
		if (getMovementState()==MovementState::Crouching)
			camera.setZ(movementData.parameters.crouchHeight);

		// Jumping logic
		if (getMovementState()==MovementState::Jumping) {
			MicroSeconds timeDelta=microSecondsGet()-movementData.jumping.startTime;

			// Finished jumping?
			if (timeDelta>=movementData.parameters.jumpTime)
				movementData.state=MovementState::Standard;
			else {
				// Otherwise update camera Z value based on how far through jump we are
				double jumpTimeFraction=((double)timeDelta)/movementData.parameters.jumpTime;
				double currJumpHeight=sin(jumpTimeFraction*M_PI)*movementData.parameters.jumpHeight;
				camera.setZ(movementData.parameters.standHeight+currJumpHeight);
			}
		}
	}
};
