#ifndef RAYCAST_RENDERER_H
#define RAYCAST_RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "camera.h"
#include "colour.h"
#include "ray.h"

namespace RayCast {

	class Renderer {
	public:
		struct BlockInfo {
			double height;
			Colour colour; // should always be set - used for top of block at the very least
			SDL_Texture *texture; // texture for block walls, if NULL then colour is used instead
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

			int blockDisplayBase;
			int blockDisplayHeight;

			int blockDisplayTopSize; // only defined if top is visible

			int blockTextureX; // only defined if block's texture!=NULL

			BlockInfo blockInfo;
		};

		SDL_Renderer *renderer;
		int windowWidth;
		int windowHeight;
		GetBlockInfoFunctor *getBlockInfoFunctor;

		Colour colourBg, colourGround, colourSky;

		static int computeDisplayHeight(const double &blockHeight, const double &distance);

		double colourDistanceFactor(double distance) const ;
		void colourAdjustForDistance(Colour &colour, double distance) const ;
	};
};

#endif
