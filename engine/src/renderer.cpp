#include <cassert>
#include <cmath>
#include <limits>
#include <algorithm>

#include <SDL2/SDL2_gfxPrimitives.h>

#include "ray.h"
#include "renderer.h"

namespace TremorEngine {
	struct RendererCompareObjectsByDistance {
		RendererCompareObjectsByDistance(const Camera &camera): camera(camera) {
		}

		~RendererCompareObjectsByDistance() {
		}

		bool operator() (Object *i, Object *j) {
			double iDx=i->getCamera().getX()-camera.getX();
			double iDy=i->getCamera().getY()-camera.getY();
			double iDist2=(iDx*iDx+iDy*iDy);

			double jDx=j->getCamera().getX()-camera.getX();
			double jDy=j->getCamera().getY()-camera.getY();
			double jDist2=(jDx*jDx+jDy*jDy);

			return (iDist2>=jDist2);
		}

		const Camera &camera;
	};

	Renderer::Renderer(SDL_Renderer *renderer, int windowWidth, int windowHeight, double unitBlockHeight, GetBlockInfoFunctor *getBlockInfoFunctor, void *getBlockInfoUserData, GetObjectsInRangeFunctor *getObjectsInRangeFunctor, void *getObjectsInRangeUserData): renderer(renderer), windowWidth(windowWidth), windowHeight(windowHeight), unitBlockHeight(unitBlockHeight), getBlockInfoFunctor(getBlockInfoFunctor), getBlockInfoUserData(getBlockInfoUserData), getObjectsInRangeFunctor(getObjectsInRangeFunctor), getObjectsInRangeUserData(getObjectsInRangeUserData) {
		colourBg.r=255; colourBg.g=0; colourBg.b=255; colourBg.a=255; // Pink (to help identify any undrawn regions).
		colourGround.r=0; colourGround.g=255; colourGround.b=0; colourGround.a=255; // Green.
		colourSky.r=0; colourSky.g=0; colourSky.b=255; colourSky.a=255; // Blue.

		zBuffer=(double *)malloc(sizeof(double)*windowWidth*windowHeight);
	}

	Renderer::~Renderer() {
		free(zBuffer);
	}

	void Renderer::render(const Camera &camera, bool drawZBuffer) {
		// Calculate various useful values.
		double screenDist=camera.getScreenDistance(windowWidth);

		int cameraZScreenAdjustment=(camera.getZ()-0.5)*unitBlockHeight;

		double cameraPitchScreenAdjustmentDouble=tan(camera.getPitch())*screenDist;
		if (cameraPitchScreenAdjustmentDouble>windowHeight)
			cameraPitchScreenAdjustmentDouble=windowHeight;
		if (cameraPitchScreenAdjustmentDouble<-windowHeight)
			cameraPitchScreenAdjustmentDouble=-windowHeight;
		int cameraPitchScreenAdjustment=cameraPitchScreenAdjustmentDouble;

		int horizonHeight=windowHeight/2+cameraPitchScreenAdjustment;

		// Clear z-buffer to infinity values.
		for(unsigned i=0; i<windowWidth*windowHeight; ++i)
			zBuffer[i]=std::numeric_limits<double>::max();

		// Clear surface.
		SDL_SetRenderDrawColor(renderer, colourBg.r, colourBg.g, colourBg.b, colourBg.a);
		SDL_Rect rect={0, 0, windowWidth, windowHeight};
		SDL_RenderFillRect(renderer, &rect);

		// Draw sky and ground.
		int y;
		for(y=0;y<windowHeight;++y) {
			Colour colour;

			if (y<horizonHeight) {
				// Calculate distance in order to adjust colour.
				double distance=unitBlockHeight/(2*(horizonHeight-y));

				// Sky
				colour=colourSky;
				colourAdjustForDistance(colour, distance);
				SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
				SDL_RenderDrawLine(renderer, 0, y, windowWidth, y);
			} else {
				// Calculate distance in order to adjust colour.
				double distance=unitBlockHeight/(2*(y-horizonHeight));

				// Ground
				colour=colourGround;
				colourAdjustForDistance(colour, distance);
				SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
				SDL_RenderDrawLine(renderer, 0, y, windowWidth, y);
			}
		}

		// Draw blocks.
		// Loop over each vertical slice of the screen.
		int x;
		for(x=0;x<windowWidth;++x) {
			// Trace ray from view point at this angle to collect a list of 'slices' of blocks to later draw.
			double deltaAngle=atan((x-windowWidth/2)/screenDist);
			double angle=camera.getYaw()+deltaAngle;
			Ray ray(camera.getX(), camera.getY(), angle);

			#define SlicesMax 64
			BlockDisplaySlice slices[SlicesMax];
			size_t slicesNext=0;

			ray.next(); // advance ray to first intersection point
			while(ray.getTrueDistance()<camera.getMaxDist()) {
				// Get info for block at current ray position.
				int mapX=ray.getMapX();
				int mapY=ray.getMapY();
				if (!getBlockInfoFunctor(mapX, mapY, &slices[slicesNext].blockInfo, getBlockInfoUserData)) {
					ray.next(); // advance ray here as we skip proper advancing futher in loop body
					continue; // no block
				}

				// We have already added blockInfo to slice stack, so add and compute other fields now.
				slices[slicesNext].distance=ray.getTrueDistance();
				slices[slicesNext].intersectionSide=ray.getSide();
				slices[slicesNext].blockDisplayBase=computeBlockDisplayBase(slices[slicesNext].distance, cameraZScreenAdjustment, cameraPitchScreenAdjustment);
				slices[slicesNext].blockDisplayHeight=computeBlockDisplayHeight(slices[slicesNext].blockInfo.height, slices[slicesNext].distance);
				if (slices[slicesNext].blockInfo.texture!=NULL) {
					int textureW=slices[slicesNext].blockInfo.texture->getWidth();
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
				if (blockDisplayTop>horizonHeight) {
					double nextDistance=ray.getTrueDistance();
					int nextBlockDisplayBase=computeBlockDisplayBase(nextDistance, cameraZScreenAdjustment, cameraPitchScreenAdjustment);
					int nextBlockDisplayHeight=computeBlockDisplayHeight(slices[slicesNext].blockInfo.height, nextDistance);
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
				if (!drawZBuffer) {
					if (slices[slicesNext].blockInfo.texture!=NULL) {
						// Textured block
						// Note: we always ask the renderer to draw the whole slice - even if say we are very close to a block and so most of the slice is not visible anyway - hoping that it interally optimises the blit.

						uint8_t colourMod=(slices[slicesNext].intersectionSide==Ray::Side::Horizontal ? 153 : 255);
						SDL_SetTextureColorMod(slices[slicesNext].blockInfo.texture->getSdlTexture(), colourMod, colourMod, colourMod); // make edges/corners between horizontal and vertical walls clearer

						SDL_Rect srcRect={.x=slices[slicesNext].blockTextureX, .y=0, .w=1, .h=slices[slicesNext].blockInfo.texture->getHeight()};
						SDL_Rect destRect={.x=x, .y=slices[slicesNext].blockDisplayBase-slices[slicesNext].blockDisplayHeight, .w=1, .h=slices[slicesNext].blockDisplayHeight};
						SDL_RenderCopy(renderer, slices[slicesNext].blockInfo.texture->getSdlTexture(), &srcRect, &destRect);
					} else {
						// Solid colour block

						// Calculate display colour for block
						Colour blockDisplayColour=slices[slicesNext].blockInfo.colour;
						if (slices[slicesNext].intersectionSide==Ray::Side::Horizontal)
							blockDisplayColour.mul(0.6); // make edges/corners between horizontal and vertical walls clearer
						colourAdjustForDistance(blockDisplayColour, slices[slicesNext].distance);

						// Draw block
						SDL_SetRenderDrawColor(renderer, blockDisplayColour.r, blockDisplayColour.g, blockDisplayColour.b, blockDisplayColour.a);
						SDL_RenderDrawLine(renderer, x, slices[slicesNext].blockDisplayBase-slices[slicesNext].blockDisplayHeight, x, slices[slicesNext].blockDisplayBase);
					}
				}

				// Update z-buffer
				int zBufferYLoopStart=slices[slicesNext].blockDisplayBase-slices[slicesNext].blockDisplayHeight;
				if (zBufferYLoopStart<0)
					zBufferYLoopStart=0;
				int zBufferYLoopEnd=slices[slicesNext].blockDisplayBase;
				if (zBufferYLoopEnd>=windowHeight)
					zBufferYLoopEnd=windowHeight-1;
				for(int y=zBufferYLoopStart; y<=zBufferYLoopEnd; ++y) {
					assert(slices[slicesNext].distance<zBuffer[x+y*windowWidth]);
					zBuffer[x+y*windowWidth]=slices[slicesNext].distance;
				}

				// Do we need to draw top of this block? (because it is below the horizon)
				int blockDisplayTop=slices[slicesNext].blockDisplayBase-slices[slicesNext].blockDisplayHeight;
				if (blockDisplayTop>horizonHeight) {
					if (!drawZBuffer) {
						Colour blockTopDisplayColour=slices[slicesNext].blockInfo.colour;
						blockTopDisplayColour.mul(1.05);
						SDL_SetRenderDrawColor(renderer, blockTopDisplayColour.r, blockTopDisplayColour.g, blockTopDisplayColour.b, blockTopDisplayColour.a);
						SDL_RenderDrawLine(renderer, x, blockDisplayTop-slices[slicesNext].blockDisplayTopSize, x, blockDisplayTop);
					}

					// Update z-buffer
					int zBufferYLoopStart=blockDisplayTop-slices[slicesNext].blockDisplayTopSize;
					if (zBufferYLoopStart<0)
						zBufferYLoopStart=0;
					int zBufferYLoopEnd=blockDisplayTop;
					if (zBufferYLoopEnd>=windowHeight)
						zBufferYLoopEnd=windowHeight-1;
					for(int y=zBufferYLoopStart; y<=zBufferYLoopEnd; ++y) {
						// Note: this is not correct - the distance should start at the one used below,
						// but then increase up to the ray's distance at next intersection point,
						// as calculated in ray casting step. However this should be safe for the purposes
						// of using the z-buffer for drawing sprites, which should be above the floor/tops
						// anyway.
						zBuffer[x+y*windowWidth]=slices[slicesNext].distance;
					}
				}
			}

			#undef SlicesMax
		}

		// Draw object sprites
		std::vector<Object *> *objects=getObjectsInRangeFunctor(camera, getObjectsInRangeUserData);

		RendererCompareObjectsByDistance compareObjectsByDistance(camera);
		std::sort(objects->begin(), objects->end(), compareObjectsByDistance); // sort so that we paint closer objects over the top of further away ones (the z buffer is not enough if textures are partially transparent)

		for(auto object : *objects) {
			// Determine angle from camera to object, and skip drawing if object is behind camera.
			double dx=camera.getX()-object->getCamera().getX();
			double dy=camera.getY()-object->getCamera().getY();
			double objectAngleDirect=atan2(dy, dx);
			double objectAngleRelative=angleNormalise(objectAngleDirect-camera.getYaw());
			if (objectAngleRelative<0.5*M_PI || objectAngleRelative>1.5*M_PI)
				continue;

			// Determine x-coordinate of screen where object should appear, and its width.
			// Skip drawing if zero-width or off screen (too far left or right).
			double distance=sqrt(dx*dx+dy*dy);
			int objectCentreScreenX=tan(objectAngleRelative)*screenDist+windowWidth/2;
			int objectScreenW=computeBlockDisplayHeight(object->getWidth(), distance);
			if (objectScreenW<=0 || objectCentreScreenX+objectScreenW/2<0 || objectCentreScreenX-objectScreenW/2>=windowWidth)
				continue;

			// Compute base and height of object on screen, and skip drawing if zero-height or off screen (too high/low).
			int objectScreenBase=computeBlockDisplayBase(distance, cameraZScreenAdjustment, cameraPitchScreenAdjustment);
			int objectScreenH=computeBlockDisplayHeight(object->getHeight(), distance);
			if (objectScreenH<=0 || objectScreenBase<0 || objectScreenBase-objectScreenH>=windowHeight)
				continue;

			// Grab texture info and compute factors used to map screen pixels to texture pixels.
			double objectVisibleAngle=object->getCamera().getYaw()+objectAngleDirect;
			Texture *objectTexture=object->getTextureAngle(objectVisibleAngle);
			if (objectTexture==NULL)
				continue;

			double textureXFactor=((double)objectTexture->getWidth())/objectScreenW;
			double textureYFactor=((double)objectTexture->getHeight())/objectScreenH;

			// Loop over all pixels in the w/h region, deciding whether to paint each one.
			// Loop over y values
			for(int ty=0, sy=objectScreenBase-objectScreenH; ty<objectScreenH; ++ty, ++sy) {
				// Column off screen?
				if (sy<0 || sy>=windowHeight)
					continue;

				// Loop over x values
				int textureExtractY=ty*textureYFactor;
				for(int tx=0, sx=objectCentreScreenX-objectScreenW/2; tx<objectScreenW; ++tx, ++sx) {
					// Pixel off screen?
					if (sx<0 || sx>=windowWidth)
						continue;

					// z-buffer indicates object would not be visible?
					if (distance>zBuffer[sx+sy*windowWidth])
						continue;

					// Update z-buffer (no need if not drawing it - we already draw objects back-to-front anyway)
					if (drawZBuffer)
						zBuffer[sx+sy*windowWidth]=distance;

					// Draw pixel
					if (!drawZBuffer) {
						int textureExtractX=tx*textureXFactor;
						Colour pixel=objectTexture->getPixel(textureExtractX, textureExtractY);
						SDL_SetRenderDrawColor(renderer, pixel.r, pixel.g, pixel.b, pixel.a);
						SDL_RenderDrawPoint(renderer, sx, sy);
					}
				}
			}
		}

		delete objects;

		// If needed draw z-buffer
		if (drawZBuffer) {
			for(int y=0; y<windowHeight; ++y) {
				for(int x=0; x<windowWidth; ++x) {
					double distance=zBuffer[x+y*windowWidth];
					double factor=(distance>1.0 ? 1.0/distance : 1.0);
					uint8_t colour=(int)floor(255*factor);

					SDL_SetRenderDrawColor(renderer, colour, colour, colour, 255);
					SDL_RenderDrawPoint(renderer, x, y);
				}
			}
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
				if (!getBlockInfoFunctor(x, y, &blockInfo, getBlockInfoUserData))
					continue;

				// Draw block
				SDL_SetRenderDrawColor(renderer, blockInfo.colour.r, blockInfo.colour.g, blockInfo.colour.b, blockInfo.colour.a);
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
		Ray ray(camera.getX(), camera.getY(), camera.getYaw());
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
		SDL_RenderDrawLine(renderer, SX(camera.getX()), SY(camera.getY()), SX(camera.getX()+cos(camera.getYaw())*len), SY(camera.getY()+sin(camera.getYaw())*len));

		#undef SX
		#undef SY
	}

	int Renderer::computeBlockDisplayBase(double distance, int cameraZScreenAdjustment, int cameraPitchScreenAdjustment) {
		int unitBlockDisplayHeight=computeBlockDisplayHeight(1.0, distance);
		return (windowHeight+unitBlockDisplayHeight)/2+cameraZScreenAdjustment/distance+cameraPitchScreenAdjustment;
	}

	int Renderer::computeBlockDisplayHeight(double blockHeightFraction, double distance) {
		return (distance>0.0 ? (blockHeightFraction*unitBlockHeight)/distance : std::numeric_limits<double>::max());
	}

	double Renderer::colourDistanceFactor(double distance) const {
		return (distance>1.0 ? 1.0/sqrt(distance) : 1.0);
	}

	void Renderer::colourAdjustForDistance(Colour &colour, double distance) const {
		colour.mul(colourDistanceFactor(distance));
	}
};
