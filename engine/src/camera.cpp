#include <cmath>

#include "camera.h"

namespace TremorEngine {

	Camera::Camera(double x, double y, double z, double yaw, double gPitch, double fov, double maxDist): x(x), y(y), z(z), yaw(yaw), fov(fov), maxDist(maxDist) {
		setPitch(gPitch);
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

	void Camera::strafe(double delta) {
		x+=-sin(yaw)*delta;
		y+=cos(yaw)*delta;
	}

	void Camera::turn(double delta) {
		yaw+=delta;
	}

	void Camera::pitch(double delta) {
		setPitch(pitchValue+delta);
	}
};
