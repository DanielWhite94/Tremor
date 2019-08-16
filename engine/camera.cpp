#include "camera.h"

namespace RayCast {

	Camera::Camera(double X, double Y, double Angle, double Fov, double MaxDist) {
		x=X;
		y=Y;
		angle=Angle;
		fov=Fov;
		maxDist=MaxDist;
	}

	double Camera::getX(void) const {
		return x;
	}

	double Camera::getY(void) const {
		return y;
	}

	double Camera::getAngle(void) const {
		return angle;
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

	void Camera::setAngle(double newAngle) {
		angle=newAngle;
	}

	void Camera::move(double delta) {
		x+=cos(angle)*delta;
		y+=sin(angle)*delta;
	}

	void Camera::turn(double delta) {
		angle+=delta;
	}
};
