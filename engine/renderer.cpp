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
			// Trace ray from view point at this angle.
			double deltaAngle=atan((x-windowWidth/2)/screenDist);
			double angle=camera.getAngle()+deltaAngle;
			Ray ray(camera.getX(), camera.getY(), angle);

			for(double stepCount=0;stepCount<camera.
			getMaxDist();++stepCount) { // FIXME: This is just a hack (the distance is limited but not correctly).
				// Advance ray to next (or first) intersection point.
				ray.next();

				// Get info for block at current ray position.
				int mapX=ray.getMapX();
				int mapY=ray.getMapY();
				BlockInfo blockInfo;
				if (!getBlockInfoFunctor(mapX, mapY, &blockInfo))
					continue;

				// Calculate display height for block from 'true distance'
				double distance=ray.getTrueDistance();

				const double unitBlockHeight=512.0;
				int unitBlockDisplayHeight=this->computeDisplayHeight(unitBlockHeight, distance);
				int blockDisplayBase=(windowHeight+unitBlockDisplayHeight)/2;

				double blockTrueHeight=blockInfo.height*unitBlockHeight;
				int blockDisplayHeight=this->computeDisplayHeight(blockTrueHeight, distance);
				if (blockDisplayBase-blockDisplayHeight<0)
					blockDisplayHeight=blockDisplayBase;

				// Calculate display colour for block
				Colour blockDisplayColour={.r=255, .g=0, .b=0}; // TODO: Get from BlockInfo
				if (ray.getSide())
					blockDisplayColour.mul(0.7); // make edges/corners between horizontal and vertical walls clearer
				colourAdjustForDistance(blockDisplayColour, distance);

				// Draw block
				SDL_SetRenderDrawColor(renderer, blockDisplayColour.r, blockDisplayColour.g, blockDisplayColour.b, 255);
				SDL_RenderDrawLine(renderer, x, blockDisplayBase-blockDisplayHeight, x, blockDisplayBase);

				// Move onto next ray.
				break;
			}
		}
	}

	void Renderer::renderTopDown(const Camera &camera) {
		#define SX(X) ((int)(windowWidth/2+cellW*(camera.getX()-(X))))
		#define SY(Y) ((int)(windowHeight/2+cellH*(camera.getY()-(Y))))

		const int cellW=16;
		const int cellH=16;

		// Clear screen.
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_Rect rect={0, 0, windowWidth, windowHeight};
		SDL_RenderFillRect(renderer, &rect);

		// Debugging grid.
		int cellsWide=windowWidth/cellW+2;
		int cellsHigh=windowHeight/cellH+2;
		int minMapX=floor(camera.getX())-cellsWide/2;
		int minMapY=floor(camera.getY())-cellsHigh/2;
		int maxMapX=minMapX+cellsWide;
		int maxMapY=minMapY+cellsHigh;
		SDL_SetRenderDrawColor(renderer, 196, 196, 196, 255);
		int x, y;
		for(x=minMapX;x<=maxMapX;++x)
			SDL_RenderDrawLine(renderer, SX(x), 0, SX(x), windowHeight);
		for(y=minMapY;y<=maxMapY;++y)
			SDL_RenderDrawLine(renderer, 0, SY(y), windowWidth, SY(y));

		// Trace ray.
		Ray ray(camera.getX(), camera.getY(), camera.getAngle());
		int i;
		for(i=0;i<64;++i) {
			// Draw highlighted cell.
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
			SDL_Rect rect={SX(ray.getMapX())+1, SY(ray.getMapY())+1, cellW-1, cellH-1};
			SDL_RenderFillRect(renderer, &rect);

			// Advance ray.
			ray.next();
		}

		// Camera cross-hair.
		int k=2;
		SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
		SDL_RenderDrawLine(renderer, SX(camera.getX())-k, SY(camera.getY())-k, SX(camera.getX())+k, SY(camera.getY())+k);
		SDL_RenderDrawLine(renderer, SX(camera.getX())-k, SY(camera.getY())+k, SX(camera.getX())+k, SY(camera.getY())-k);

		// Line of sight.
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
