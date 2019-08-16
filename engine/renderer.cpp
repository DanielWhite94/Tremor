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
		// Calculate various useful values.
		double screenDist=windowWidth/(2*tan(camera.getFov()/2));

		// Clear surface.
		SDL_SetRenderDrawColor(renderer, colourBg.r, colourBg.g, colourBg.b, 255);
		SDL_Rect rect={0, 0, windowWidth, windowHeight};
		SDL_RenderFillRect(renderer, &rect);

		// Draw sky and ground.
		int y;
		for(y=0;y<windowHeight/2;++y) {
			// Calculate distance in order to adjust colour.
			double distance=blockBaseHeight/(windowHeight-2*y);
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

			while(ray.getTrueDistance()<camera.getMaxDist()) {
				// Advance ray to next (or first) intersection point.
				ray.next();

				// Get info for block at current ray position.
				int mapX=ray.getMapX();
				int mapY=ray.getMapY();
				if (!getBlockInfoFunctor(mapX, mapY, &slices[slicesNext].blockInfo))
					continue; // no block

				// If this block is no taller than a one already found, then it will be hidden anyway so don't bother adding to slice stack.
				// FIXME: this logic will break if we end up supporting mapping textures with transparency onto blocks
				if (slicesNext>0 && slices[slicesNext].blockInfo.height<=slices[slicesNext-1].blockInfo.height)
					continue;

				// We have already added blockInfo to slice stack, so add other fields now.
				slices[slicesNext].distance=ray.getTrueDistance();
				slices[slicesNext].intersectionSide=ray.getSide();
				++slicesNext;
			}

			// Loop over found blocks in reverse
			while(slicesNext>0) {
				// Adjust slicesNext now due to how it usually points one beyond last entry
				--slicesNext;

				// Calculate display height for this block from 'true distance'
				const double unitBlockHeight=1024.0;
				int unitBlockDisplayHeight=this->computeDisplayHeight(unitBlockHeight, slices[slicesNext].distance);
				int blockDisplayBase=(windowHeight+unitBlockDisplayHeight)/2;

				double blockTrueHeight=slices[slicesNext].blockInfo.height*unitBlockHeight;
				int blockDisplayHeight=this->computeDisplayHeight(blockTrueHeight, slices[slicesNext].distance);
				if (blockDisplayBase-blockDisplayHeight<0)
					blockDisplayHeight=blockDisplayBase;

				// Calculate display colour for block
				Colour blockDisplayColour=slices[slicesNext].blockInfo.colour;
				if (slices[slicesNext].intersectionSide==Ray::Side::Horizontal)
					blockDisplayColour.mul(0.7); // make edges/corners between horizontal and vertical walls clearer
				colourAdjustForDistance(blockDisplayColour, slices[slicesNext].distance);

				// Draw block
				SDL_SetRenderDrawColor(renderer, blockDisplayColour.r, blockDisplayColour.g, blockDisplayColour.b, 255);
				SDL_RenderDrawLine(renderer, x, blockDisplayBase-blockDisplayHeight, x, blockDisplayBase);
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

	double Renderer::computeDisplayHeight(const double &blockHeight, const double &distance) {
		return (distance>0 ? blockHeight/distance : std::numeric_limits<double>::max());
	}

	double Renderer::colourDistanceFactor(double distance) const {
		return (distance>=1 ? 1/sqrt(distance) : 1);
	}

	void Renderer::colourAdjustForDistance(Colour &colour, double distance) const {
		colour.mul(colourDistanceFactor(distance));
	}
};
