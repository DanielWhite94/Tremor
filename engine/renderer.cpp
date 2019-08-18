#include <cmath>
#include <limits>

#include <SDL2/SDL2_gfxPrimitives.h>

#include "ray.h"
#include "renderer.h"

namespace RayCast {

	Renderer::Renderer(SDL_Renderer *grenderer, int gwindowWidth, int gwindowHeight, GetBlockInfoFunctor *ggetBlockInfoFunctor) {
		renderer=grenderer;
		windowWidth=gwindowWidth;
		windowHeight=gwindowHeight;
		getBlockInfoFunctor=ggetBlockInfoFunctor;

		colourBg.r=255; colourBg.g=0; colourBg.b=255; // Pink (to help identify any undrawn regions).
		colourGround.r=0; colourGround.g=255; colourGround.b=0; // Green.
		colourSky.r=0; colourSky.g=0; colourSky.b=255; // Blue.
	}

	Renderer::~Renderer() {
	}

	void Renderer::render(const Camera &camera) {
		// Parameters
		const double unitBlockHeight=1024.0; // increasing this will stretch blocks to be larger vertically relative to their width, decreasing will shrink them

		// Calculate various useful values.
		double screenDist=windowWidth/(2*tan(camera.getFov()/2));

		int cameraZScreenAdjustment=(camera.getZ()-0.5)*unitBlockHeight;

		// Clear surface.
		SDL_SetRenderDrawColor(renderer, colourBg.r, colourBg.g, colourBg.b, 255);
		SDL_Rect rect={0, 0, windowWidth, windowHeight};
		SDL_RenderFillRect(renderer, &rect);

		// Draw sky and ground.
		int y;
		for(y=0;y<windowHeight/2;++y) {
			// Calculate distance in order to adjust colour.
			double distance=unitBlockHeight/(windowHeight-2*y);
			Colour colour;

			// Sky
			colour=colourSky;
			colourAdjustForDistance(colour, distance);
			SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, 255);
			SDL_RenderDrawLine(renderer, 0, y, windowWidth, y);

			// Ground
			colour=colourGround;
			colourAdjustForDistance(colour, distance);
			SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, 255);
			SDL_RenderDrawLine(renderer, 0, windowHeight-1-y, windowWidth, windowHeight-1-y);
		}

		// Draw blocks.
		// Loop over each vertical slice of the screen.
		int x;
		for(x=0;x<windowWidth;++x) {
			// Trace ray from view point at this angle to collect a list of 'slices' of blocks to later draw.
			double deltaAngle=atan((x-windowWidth/2)/screenDist);
			double angle=camera.getAngle()+deltaAngle;
			Ray ray(camera.getX(), camera.getY(), angle);

			#define SlicesMax 64
			BlockDisplaySlice slices[SlicesMax];
			size_t slicesNext=0;

			ray.next(); // advance ray to first intersection point
			while(ray.getTrueDistance()<camera.getMaxDist()) {
				// Get info for block at current ray position.
				int mapX=ray.getMapX();
				int mapY=ray.getMapY();
				if (!getBlockInfoFunctor(mapX, mapY, &slices[slicesNext].blockInfo)) {
					ray.next(); // advance ray here as we skip proper advancing futher in loop body
					continue; // no block
				}

				// We have already added blockInfo to slice stack, so add and compute other fields now.
				slices[slicesNext].distance=ray.getTrueDistance();
				slices[slicesNext].intersectionSide=ray.getSide();

				int unitBlockDisplayHeight=this->computeDisplayHeight(unitBlockHeight, slices[slicesNext].distance);
				slices[slicesNext].blockDisplayBase=(windowHeight+unitBlockDisplayHeight)/2;

				double cameraZDistanceFactor=1.0/slices[slicesNext].distance;
				slices[slicesNext].blockDisplayBase+=cameraZDistanceFactor*cameraZScreenAdjustment;

				double blockTrueHeight=slices[slicesNext].blockInfo.height*unitBlockHeight;
				slices[slicesNext].blockDisplayHeight=this->computeDisplayHeight(blockTrueHeight, slices[slicesNext].distance);

				if (slices[slicesNext].blockInfo.texture!=NULL) {
					int textureW;
					SDL_QueryTexture(slices[slicesNext].blockInfo.texture, NULL, NULL, &textureW, NULL);
					slices[slicesNext].blockTextureX=ray.getTextureX(textureW);
				}

				// If this block occupies whole column already, no point searching further.
				// FIXME: this logic will break if we end up supporting mapping textures with transparency onto blocks
				if (slices[slicesNext].blockDisplayHeight==slices[slicesNext].blockDisplayBase) {
					++slicesNext;
					break;
				}

				// Advance ray to next itersection now ready for next iteration, and for use in block top calculations.
				ray.next();

				// If top of block is visible, compute some extra stuff.
				int blockDisplayTop=slices[slicesNext].blockDisplayBase-slices[slicesNext].blockDisplayHeight;
				if (blockDisplayTop>windowHeight/2) {
					double nextDistance=ray.getTrueDistance();

					int unitNextBlockDisplayHeight=this->computeDisplayHeight(unitBlockHeight, nextDistance);
					int nextBlockDisplayBase=(windowHeight+unitNextBlockDisplayHeight)/2;

					double nextCameraZDistanceFactor=1.0/nextDistance;
					nextBlockDisplayBase+=nextCameraZDistanceFactor*cameraZScreenAdjustment;

					double nextBlockTrueHeight=slices[slicesNext].blockInfo.height*unitBlockHeight;
					int nextBlockDisplayHeight=this->computeDisplayHeight(nextBlockTrueHeight, nextDistance);

					int nextBlockDisplayTop=nextBlockDisplayBase-nextBlockDisplayHeight;
					slices[slicesNext].blockDisplayTopSize=blockDisplayTop-nextBlockDisplayTop;
				}

				// Push slice to stack
				++slicesNext;
			}

			// Loop over found blocks in reverse
			while(slicesNext>0) {
				// Adjust slicesNext now due to how it usually points one beyond last entry
				--slicesNext;

				// Draw block
				if (slices[slicesNext].blockInfo.texture!=NULL) {
					// Textured block
					// Note: we always ask the renderer to draw the whole slice - even if say we are very close to a block and so most of the slice is not visible anyway - hoping that it interally optimises the blit.

					int textureW, textureH;
					SDL_QueryTexture(slices[slicesNext].blockInfo.texture, NULL, NULL, &textureW, &textureH);

					SDL_Rect srcRect={.x=slices[slicesNext].blockTextureX, .y=0, .w=1, .h=textureH};
					SDL_Rect destRect={.x=x, .y=slices[slicesNext].blockDisplayBase-slices[slicesNext].blockDisplayHeight, .w=1, .h=slices[slicesNext].blockDisplayHeight};
					SDL_RenderCopy(renderer, slices[slicesNext].blockInfo.texture, &srcRect, &destRect);
				} else {
					// Solid colour block

					// Calculate display colour for block
					Colour blockDisplayColour=slices[slicesNext].blockInfo.colour;
					if (slices[slicesNext].intersectionSide==Ray::Side::Horizontal)
						blockDisplayColour.mul(0.7); // make edges/corners between horizontal and vertical walls clearer
					colourAdjustForDistance(blockDisplayColour, slices[slicesNext].distance);

					// Draw block
					SDL_SetRenderDrawColor(renderer, blockDisplayColour.r, blockDisplayColour.g, blockDisplayColour.b, 255);
					SDL_RenderDrawLine(renderer, x, slices[slicesNext].blockDisplayBase-slices[slicesNext].blockDisplayHeight, x, slices[slicesNext].blockDisplayBase);
				}

				// Do we need to draw top of this block? (because it is below the horizon)
				int blockDisplayTop=slices[slicesNext].blockDisplayBase-slices[slicesNext].blockDisplayHeight;
				if (blockDisplayTop>windowHeight/2) {
					Colour blockTopDisplayColour=slices[slicesNext].blockInfo.colour;
					blockTopDisplayColour.mul(1.05);
					SDL_SetRenderDrawColor(renderer, blockTopDisplayColour.r, blockTopDisplayColour.g, blockTopDisplayColour.b, 255);
					SDL_RenderDrawLine(renderer, x, blockDisplayTop-slices[slicesNext].blockDisplayTopSize, x, blockDisplayTop);
				}
			}

			#undef SlicesMax
		}
	}

	void Renderer::renderTopDown(const Camera &camera) {
		#define SX(X) (((int)(windowWidth/2+cellW*(camera.getX()-(X))))/divisor+xOffset)
		#define SY(Y) (((int)(windowHeight/2+cellH*(camera.getY()-(Y))))/divisor+yOffset)

		// Parameters
		const int xOffset=0;
		const int yOffset=0;
		const int divisor=4;
		const int cellW=16*divisor;
		const int cellH=16*divisor;

		// Calculate constants
		int cellsWide=windowWidth/cellW;
		int cellsHigh=windowHeight/cellH;
		int minMapX=ceil(camera.getX())-cellsWide/2;
		int minMapY=ceil(camera.getY())-cellsHigh/2;
		int maxMapX=floor(camera.getX())+cellsWide/2;
		int maxMapY=floor(camera.getY())+cellsHigh/2;
		int x, y;

		// Clear screen
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_Rect rect={xOffset, yOffset, (cellsWide*cellW)/divisor, (cellsHigh*cellH)/divisor};
		SDL_RenderFillRect(renderer, &rect);

		// Draw blocks
		for(y=minMapY;y<=maxMapY;++y) {
			for(x=minMapX;x<=maxMapX;++x) {
				// Grab block
				BlockInfo blockInfo;
				if (!getBlockInfoFunctor(x, y, &blockInfo))
					continue;

				// Draw block
				SDL_SetRenderDrawColor(renderer, blockInfo.colour.r, blockInfo.colour.g, blockInfo.colour.b, 255);
				SDL_Rect rect={SX(x), SY(y), cellW/divisor, cellH/divisor};
				SDL_RenderFillRect(renderer, &rect);
			}
		}

		// Draw grid over blocks
		SDL_SetRenderDrawColor(renderer, 196, 196, 196, 255);
		for(x=minMapX;x<=maxMapX;++x)
			SDL_RenderDrawLine(renderer, SX(x), yOffset, SX(x), windowHeight/divisor+yOffset);
		for(y=minMapY;y<=maxMapY;++y)
			SDL_RenderDrawLine(renderer, xOffset, SY(y), windowWidth/divisor+xOffset, SY(y));

		// Trace ray and highlight cells it intersects
		Ray ray(camera.getX(), camera.getY(), camera.getAngle());
		int i;
		for(i=0;i<64;++i) {
			x=ray.getMapX();
			y=ray.getMapY();

			// Out of grid?
			if (x<minMapX || x>=maxMapX || y<minMapY || y>=maxMapY)
				break;

			// Draw highlighted cell.
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
			SDL_Rect rect={SX(x)+1, SY(y)+1, (cellW-1)/divisor, (cellH-1)/divisor};
			SDL_RenderFillRect(renderer, &rect);

			// Advance ray.
			ray.next();
		}

		// Draw cross-hair to represent camera
		int k=2;
		SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
		SDL_RenderDrawLine(renderer, SX(camera.getX())-k, SY(camera.getY())-k, SX(camera.getX())+k, SY(camera.getY())+k);
		SDL_RenderDrawLine(renderer, SX(camera.getX())-k, SY(camera.getY())+k, SX(camera.getX())+k, SY(camera.getY())-k);

		// Draw camera's line of sight
		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
		double len=64.0;
		SDL_RenderDrawLine(renderer, SX(camera.getX()), SY(camera.getY()), SX(camera.getX()+cos(camera.getAngle())*len), SY(camera.getY()+sin(camera.getAngle())*len));

		#undef SX
		#undef SY
	}

	int Renderer::computeDisplayHeight(const double &blockHeight, const double &distance) {
		return (distance>0 ? blockHeight/distance : std::numeric_limits<double>::max());
	}

	double Renderer::colourDistanceFactor(double distance) const {
		return (distance>=1 ? 1/sqrt(distance) : 1);
	}

	void Renderer::colourAdjustForDistance(Colour &colour, double distance) const {
		colour.mul(colourDistanceFactor(distance));
	}
};
