#include <cmath>
#include <limits>

#include "ray.h"

namespace RayCast {

	Ray::Ray(double x, double y, double angle) {
		startX=x;
		startY=y;

		mapX=floor(startX);
		mapY=floor(startY);

		rayDirX=cos(angle);
		rayDirY=sin(angle);

		// TODO: Use trig versions instead.
		double rayDirX2=rayDirX*rayDirX;
		double rayDirY2=rayDirY*rayDirY;
		deltaDistX=(rayDirX2>0 ? sqrt(1+rayDirY2/rayDirX2) : std::numeric_limits<double>::max());
		deltaDistY=(rayDirY2>0 ? sqrt(1+rayDirX2/rayDirY2) : std::numeric_limits<double>::max());

		if (rayDirX<0) {
			stepX=-1;
			sideDistX=(startX-mapX)*deltaDistX;
		} else {
			stepX=1;
			sideDistX=(mapX+1-startX)*deltaDistX;
		}
		if (rayDirY<0) {
			stepY=-1;
			sideDistY=(startY-mapY)*deltaDistY;
		} else {
			stepY=1;
			sideDistY=(mapY+1-startY)*deltaDistY;
		}
	}

	Ray::~Ray() {
	}

	void Ray::next(void) {
		if (sideDistX<sideDistY) {
			sideDistX+=deltaDistX;
			mapX+=stepX;
			side=0;
		} else {
			sideDistY+=deltaDistY;
			mapY+=stepY;
			side=1;
		}
	}

	int Ray::getMapX(void) const {
		return mapX+1;
	}

	int Ray::getMapY(void) const {
		return mapY+1;
	}

	double Ray::getTrueDistance(void) const {
		double distance;
		if (side==0)
			distance=(rayDirX!=0 ? fabs((mapX-startX+(1-stepX)/2)/rayDirX) : std::numeric_limits<double>::max());
		else
			distance=(rayDirY!=0 ? fabs((mapY-startY+(1-stepY)/2)/rayDirY) : std::numeric_limits<double>::max());
		return distance;
	}

	int Ray::getSide(void) const {
		return side;
	}

};
