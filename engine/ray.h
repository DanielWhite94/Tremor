#ifndef RAYCAST_RAY_H
#define RAYCAST_RAY_H

namespace RayCast {

	class Ray {
	public:
		Ray(double x, double y, double angle); // 0<=angle<2pi, in radians.
		~Ray();

		void next(void); // Advance to next intersection point.

		int getMapX(void) const ;
		int getMapY(void) const ;

		double getTrueDistance(void) const ; // 'Perpendicular' distance between ray's initial and current position.

		int getSide(void) const ; // Which type of intersection (0 - vertical, 1 - horizontal).

	private:
		double startX, startY;
		double rayDirX, rayDirY;
		int mapX, mapY; // Which cell the ray is currently 'in' (most recently intersected with).
		double sideDistX, sideDistY; // Length of ray from current position to next x (or y) side.
		double deltaDistX, deltaDistY; // Length of ray from one x (or y) side to next x (or y) side.
		int stepX, stepY; // What direction to step in x (or y) direction (either +1 or -1).
		int side;
	};

};

#endif
