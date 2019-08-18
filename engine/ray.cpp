#include <cassert>
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

		deltaDistX=(rayDirX!=0.0 ? fabs(1.0/rayDirX) : std::numeric_limits<double>::max());
		deltaDistY=(rayDirY!=0.0 ? fabs(1.0/rayDirY) : std::numeric_limits<double>::max());

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

		side=Side::None;

		updateTrueDistance();
	}

	Ray::~Ray() {
	}

	void Ray::next(void) {
		if (sideDistX<sideDistY) {
			sideDistX+=deltaDistX;
			mapX+=stepX;
			side=Side::Vertical;
		} else {
			sideDistY+=deltaDistY;
			mapY+=stepY;
			side=Side::Horizontal;
		}

		updateTrueDistance();
	}

	int Ray::getMapX(void) const {
		return mapX+1;
	}

	int Ray::getMapY(void) const {
		return mapY+1;
	}

	double Ray::getTrueDistance(void) const {
		return trueDistance;
	}

	Ray::Side Ray::getSide(void) const {
		return side;
	}

	int Ray::getTextureX(int textureW) const {
		double intersectionX;
		switch(side) {
			case Side::Vertical:
				intersectionX=startY+getTrueDistance()*rayDirY;
			break;
			case Side::Horizontal:
				intersectionX=startX+getTrueDistance()*rayDirX;
			break;
			case Side::None:
				return 0; // we have not yet hit a wall
			break;
		}
		intersectionX-=floor((intersectionX));

		int textureX=(int)floor(intersectionX*textureW);
		if(side==Side::Vertical && rayDirX > 0)
			textureX=textureW-textureX-1;
		if(side==Side::Horizontal && rayDirY < 0)
			textureX=textureW-textureX-1;

		return textureX;
	}

	void Ray::updateTrueDistance(void) {
		trueDistance=0.0;

		switch(side) {
			case Side::Vertical:
				trueDistance=(rayDirX!=0 ? fabs((mapX-startX+(1-stepX)/2)/rayDirX) : std::numeric_limits<double>::max());
			break;
			case Side::Horizontal:
				trueDistance=(rayDirY!=0 ? fabs((mapY-startY+(1-stepY)/2)/rayDirY) : std::numeric_limits<double>::max());
			break;
			case Side::None:
				trueDistance=0.0;
			break;
		}
	}
};
