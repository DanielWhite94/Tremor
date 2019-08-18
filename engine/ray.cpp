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

		side=Side::None;
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
	}

	int Ray::getMapX(void) const {
		return mapX+1;
	}

	int Ray::getMapY(void) const {
		return mapY+1;
	}

	double Ray::getTrueDistance(void) const {
		switch(side) {
			case Side::Vertical:
				return (rayDirX!=0 ? fabs((mapX-startX+(1-stepX)/2)/rayDirX) : std::numeric_limits<double>::max());
			break;
			case Side::Horizontal:
				return (rayDirY!=0 ? fabs((mapY-startY+(1-stepY)/2)/rayDirY) : std::numeric_limits<double>::max());
			break;
			case Side::None:
				return 0.0;
			break;
		}

		assert(false);
		return 0.0;
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
};
