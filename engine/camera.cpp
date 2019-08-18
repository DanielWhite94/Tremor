#include <cmath>

#include "camera.h"

namespace RayCast {

	Camera::Camera(double X, double Y, double Z, double Yaw, double Pitch, double Fov, double MaxDist) {
		x=X;
		y=Y;
		z=Z;
		yaw=Yaw;
		setPitch(Pitch);
		fov=Fov;
		maxDist=MaxDist;
	}

	double Camera::getX(void) const {
		return x;
	}

	double Camera::getY(void) const {
		return y;
	}

	double Camera::getZ(void) const {
		return z;
	}

	double Camera::getYaw(void) const {
		return yaw;
	}

	double Camera::getPitch(void) const {
		return pitchValue;
	}

	double Camera::getFov(void) const {
		return fov;
	}

	double Camera::getMaxDist(void) const {
		return maxDist;
	}

	void Camera::setX(double newX) {
		x=newX;
	}

	void Camera::setY(double newY) {
		y=newY;
	}

	void Camera::setZ(double newZ) {
		z=newZ;
	}

	void Camera::setYaw(double newYaw) {
		yaw=newYaw;
	}

	void Camera::setPitch(double newPitch) {
		if (newPitch<-M_PI/2.0)
			newPitch=-M_PI/2.0;
		if (newPitch>M_PI/2.0)
			newPitch=M_PI/2.0;
		pitchValue=newPitch;
	}

	void Camera::move(double delta) {
		x+=cos(yaw)*delta;
		y+=sin(yaw)*delta;
	}

	void Camera::turn(double delta) {
		yaw+=delta;
	}

	void Camera::pitch(double delta) {
		setPitch(pitchValue+delta);
	}
};
