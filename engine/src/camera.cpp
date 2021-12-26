#include <cmath>
#include <cstdlib>

#include "camera.h"

namespace TremorEngine {

	Camera::Camera(void): x(0.0), y(0.0), z(0.0), yaw(0.0), fov(M_PI/3.0), maxDist(64.0) {
		setPitch(0.0);
	}

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

	double Camera::getScreenDistance(int windowWidth) const {
		return windowWidth/(2.0*tan(getFov()/2.0));
	}

	void Camera::getTargetInfo(double x, double y, double yaw, double *visibleAngle, double *bearingAngle, double *distance) const {
		double dx=getX()-x;
		double dy=getY()-y;

		if (visibleAngle!=NULL)
			*visibleAngle=atan2(dy, dx)+yaw;
		if (bearingAngle!=NULL)
			*bearingAngle=atan2(dy, dx)-getYaw();
		if (distance!=NULL)
			*distance=sqrt(dx*dx+dy*dy);
	}

	void Camera::getTargetInfo(const Camera &otherCamera, double *visibleAngle, double *bearingAngle, double *distance) const {
		return getTargetInfo(otherCamera.getX(), otherCamera.getY(), otherCamera.getYaw(), visibleAngle, bearingAngle, distance);
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
		if (newPitch<this->pitchMin)
			newPitch=this->pitchMin;
		if (newPitch>this->pitchMax)
			newPitch=this->pitchMax;
		pitchValue=newPitch;
	}

	void Camera::setPitchLimits(double pitchMin, double pitchMax) {
		if (pitchMin<-M_PI/2.0)
			pitchMin=-M_PI/2.0;
		if (pitchMax>M_PI/2.0)
			pitchMax=M_PI/2.0;
		if (pitchMin<pitchMax) {
			this->pitchMin=pitchMin;
			this->pitchMax=pitchMax;
		}
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
