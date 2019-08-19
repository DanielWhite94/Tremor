#include "object.h"

namespace TremorEngine {
	Object::Object(const Camera &camera, const MovementParameters &movementParameters, SDL_Texture *texture): camera(camera), texture(texture) {
		movementData.parameters=movementParameters;
		movementData.state=MovementState::Standard;
	}

	Object::~Object() {
	}

	const Camera &Object::getCamera(void) const {
		return camera;
	}

	Object::MovementState Object::getMovementState(void) const {
		return movementData.state;
	}

	SDL_Texture *Object::getTexture(void) const {
		return texture;
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
