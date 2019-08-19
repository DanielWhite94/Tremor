#ifndef TREMORENGINE_OBJECT_H
#define TREMORENGINE_OBJECT_H

#include "camera.h"
#include "util.h"

namespace TremorEngine {

	class Object {
	public:
		struct MovementParameters {
			MicroSeconds jumpTime; // time from leaving ground to landing again
			double standHeight; // actual height when standing, fraction of unit block height - should be 0.5 for best graphics
			double crouchHeight; // actual height when crouched, fraction of unit block height
			double jumpHeight; // max vertical displacement
		};

		enum class MovementState {
			Standard,
			Crouching,
			Jumping,
		};

		struct MovementStateJumpingData {
			MicroSeconds startTime;
		};

		struct MovementData {
			MovementParameters parameters;

			MovementState state;
			union {
				MovementStateJumpingData jumping;
			};
		};

		Object(const Camera &camera, const MovementParameters &movementParameters);
		~Object();

		const Camera &getCamera(void);

		MovementState getMovementState(void);

		void move(double delta); // Move in current direction (or backwards if delta is negative).
		void strafe(double delta); // Move perpendicular to direction (left is delta is negative, right if positive).
		void turn(double delta); // Adjust yaw/rotation
		void pitch(double delta); // Adjust pitch

		bool crouch(bool value); // when enabling, fails if currently jumping, when disabling fails if not already crouched
		bool jump(void); // begins a jump, can be in state standard or crouching, only fails if already jumping

		void tick(void);
	private:
		Camera camera; // even for objects which cannot 'see' this is still used for position and rotation

		MovementData movementData;
	};

};

#endif
