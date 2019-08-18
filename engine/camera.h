#ifndef RAYCAST_CAMERA_H
#define RAYCAST_CAMERA_H

#include <cmath>

namespace RayCast {

	class Camera {
	public:
		Camera(double X, double Y, double Z, double Yaw, double Pitch=0.0, double Fov=M_PI/3.0, double MaxDist=64.0);

		double getX(void) const ;
		double getY(void) const ;
		double getZ(void) const ;
		double getYaw(void) const ;
		double getPitch(void) const ;
		double getFov(void) const ;
		double getMaxDist(void) const ;

		void setX(double newX);
		void setY(double newY);
		void setZ(double newZ);
		void setYaw(double newYaw);
		void setPitch(double newYaw);

		void move(double delta); // Move in current direction (or backwards if delta is negative).
		void turn(double delta); // adjust yaw
	private:
		double x, y, z;
		double yaw, pitch;
		double fov; // Field-of-view, radians.
		double maxDist; // Maximum viewing distance.
	};

};

#endif
