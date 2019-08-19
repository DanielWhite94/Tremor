#ifndef TREMORENGINE_CAMERA_H
#define TREMORENGINE_CAMERA_H

#include <cmath>

namespace TremorEngine {

	class Camera {
	public:
		Camera(double x, double y, double z, double yaw, double pitch=0.0, double fov=M_PI/3.0, double maxDist=64.0);

		double getX(void) const ;
		double getY(void) const ;
		double getZ(void) const ;
		double getYaw(void) const ;
		double getPitch(void) const ;
		double getFov(void) const ;
		double getMaxDist(void) const ;
		double getScreenDistance(int windowWidth) const ; // take our FOV and window width and determine how far the virtual screen must be from the camera

		void setX(double newX);
		void setY(double newY);
		void setZ(double newZ);
		void setYaw(double newYaw);
		void setPitch(double newYaw); // limited to interval (-pi/2,pi/2)

		void move(double delta); // Move in current direction (or backwards if delta is negative).
		void strafe(double delta); // Move perpendicular to direction (left is delta is negative, right if positive).
		void turn(double delta); // adjust yaw
		void pitch(double delta); // adjust pitch
	private:
		double x, y, z;
		double yaw, pitchValue;
		double fov; // Field-of-view, radians.
		double maxDist; // Maximum viewing distance.
	};

};

#endif
