#ifndef TREMORENGINE_RENDERER_H
#define TREMORENGINE_RENDERER_H

#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "camera.h"
#include "colour.h"
#include "object.h"
#include "ray.h"

namespace TremorEngine {

	class Renderer {
	public:
		struct BlockInfo {
			double height;
			Colour colour; // should always be set - used for top of block at the very least
			Texture *texture; // texture for block walls, if NULL then colour is used instead
		};

		typedef bool (GetBlockInfoFunctor)(int mapX, int mapY, BlockInfo *info, void *userData); // should return false if no such block
		typedef std::vector<Object *> * (GetObjectsInRangeFunctor)(const Camera &camera, void *userData);

		Renderer(SDL_Renderer *renderer, int windowWidth, int windowHeight, double unitBlockHeight, GetBlockInfoFunctor *getBlockInfoFunctor, void *getBlockInfoUserData, GetObjectsInRangeFunctor *getObjectsInRangeFunctor, void *getObjectsInRangeUserData);
		~Renderer();

		double getBrightnessMin(void) const;
		double getBrightnessMax(void) const;

		// brightness values should be in interval [0.0,1.0]
		void setBrightnessMin(double value);
		void setBrightnessMax(double value);

		void setGroundColour(const Colour &colour);
		void setSkyColour(const Colour &colour);

		void render(const Camera &camera, bool drawZBuffer); // if drawZBuffer is true then all standard rendering logic is carried out, and then at the very end we draw a heatmap of the z-buffer over the top
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
		double unitBlockHeight; // increasing this will stretch blocks to be larger vertically relative to their width, decreasing will shrink them
		GetBlockInfoFunctor *getBlockInfoFunctor;
		void *getBlockInfoUserData;
		GetObjectsInRangeFunctor *getObjectsInRangeFunctor;
		void *getObjectsInRangeUserData;

		Colour colourBg, colourGround, colourSky;

		// These values control the brightness levels.
		// Default values are (min,max)=(0.0,1.0).
		// Example values:
		// dim space - (0.0,0.5)
		// bright space - (0.5,1.0)
		double brightnessMin, brightnessMax;

		double *zBuffer; // windowWidth*windowHeight number of entries

		int computeBlockDisplayBase(double distance, int cameraZScreenAdjustment, int cameraPitchScreenAdjustment);
		int computeBlockDisplayHeight(double blockHeightFraction, double distance);

		double colourDistanceFactor(double distance) const ;
		void colourAdjustForDistance(Colour &colour, double distance) const ;
	};
};

#endif
