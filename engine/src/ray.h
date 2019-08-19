#ifndef TREMORENGINE_RAY_H
#define TREMORENGINE_RAY_H

namespace TremorEngine {

	class Ray {
	public:
		enum class Side {
			Vertical,
			Horizontal,
			None, // next() has not yet been called on the ray and so we have not hit any walls
		};
		Ray(double x, double y, double angle); // 0<=angle<2pi, in radians.
		~Ray();

		void next(void); // Advance to next intersection point.

		int getMapX(void) const ;
		int getMapY(void) const ;

		double getTrueDistance(void) const ; // 'Perpendicular' distance between ray's initial and current position.

		Side getSide(void) const ; // Type of of last intersection

		int getTextureX(int textureW) const; // return, as of last intersection, the x-offset into a texture rendered on this wall
	private:
		double startX, startY;
		double rayDirX, rayDirY;
		int mapX, mapY; // Which cell the ray is currently 'in' (most recently intersected with).
		double sideDistX, sideDistY; // Length of ray from current position to next x (or y) side.
		double deltaDistX, deltaDistY; // Length of ray from one x (or y) side to next x (or y) side.
		int stepX, stepY; // What direction to step in x (or y) direction (either +1 or -1).
		Side side;
		double trueDistance; // perpendicular distance from ray start to last intersection

		void updateTrueDistance(void);
	};

};

#endif
