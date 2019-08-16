#ifndef RAYCAST_CAMERA_H
#define RAYCAST_CAMERA_H

#include <cmath>

namespace RayCast {

	class Camera {
	public:
		Camera(double X, double Y, double Angle, double Fov=M_PI/3.0, double MaxDist=64.0);

		double getX(void) const ;
		double getY(void) const ;
		double getAngle(void) const ;
		double getFov(void) const ;
		double getMaxDist(void) const ;

		void setX(double newX);
		void setY(double newY);
		void setAngle(double newAngle);

		void move(double delta); // Move in current direction (or backwards if delta is negative).
		void turn(double delta);
	private:
		double x, y;
		double angle;
		double fov; // Field-of-view, radians.
		double maxDist; // Maximum viewing distance.
	};

};

#endif
