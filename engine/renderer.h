#ifndef RAYCAST_RENDERER_H
#define RAYCAST_RENDERER_H

#include <SDL2/SDL.h>

#include "camera.h"
#include "colour.h"
#include "ray.h"

namespace RayCast {

	class Renderer {
	public:
		struct BlockInfo {
			double height;
			Colour colour;
		};

		typedef bool (GetBlockInfoFunctor)(int mapX, int mapY, BlockInfo *info); // should return false if no such block

		Renderer(SDL_Renderer *renderer, int windowWidth, int windowHeight, GetBlockInfoFunctor *getBlockInfoFunctor);
		~Renderer();

		void render(const Camera &camera);
		void renderTopDown(const Camera &camera);

	private:
		struct BlockDisplaySlice {
			double distance;
			Ray::Side intersectionSide;
			BlockInfo blockInfo;
		};

		SDL_Renderer *renderer;
		int windowWidth;
		int windowHeight;
		GetBlockInfoFunctor *getBlockInfoFunctor;

		const double blockBaseHeight=64;
		Colour colourBg, colourGround, colourSky;

		static int computeDisplayHeight(const double &blockHeight, const double &distance);

		double colourDistanceFactor(double distance) const ;
		void colourAdjustForDistance(Colour &colour, double distance) const ;
	};
};

#endif
