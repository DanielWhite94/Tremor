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

		brightnessMin=0.0;
		brightnessMax=1.0;
	}

	Renderer::~Renderer() {
		free(zBuffer);
	}

	double Renderer::getBrightnessMin(void) const {
		return brightnessMin;
	}

	double Renderer::getBrightnessMax(void) const {
		return brightnessMax;
	}

	void Renderer::setBrightnessMin(double value) {
		assert(value>=0.0 && value<=1.0);

		brightnessMin=value;
	}

	void Renderer::setBrightnessMax(double value) {
		assert(value>=0.0 && value<=1.0);

		brightnessMax=value;
	}

	void Renderer::setGroundColour(const Colour &colour) {
		colourGround=colour;
	}

	void Renderer::setSkyColour(const Colour &colour) {
		colourSky=colour;
	}

	void Renderer::render(const Camera &camera, bool drawZBuffer) {
		// Init RenderData struct
		RenderData renderData={.camera=camera};
		renderData.drawZBuffer=drawZBuffer;

		// Calculate various useful values.
		renderData.screenDist=camera.getScreenDistance(windowWidth);

		renderData.cameraZScreenAdjustment=(camera.getZ()-0.5)*unitBlockHeight;

		double cameraPitchScreenAdjustmentDouble=tan(camera.getPitch())*renderData.screenDist;
		if (cameraPitchScreenAdjustmentDouble>windowHeight)
			cameraPitchScreenAdjustmentDouble=windowHeight;
		if (cameraPitchScreenAdjustmentDouble<-windowHeight)
			cameraPitchScreenAdjustmentDouble=-windowHeight;
		renderData.cameraPitchScreenAdjustment=cameraPitchScreenAdjustmentDouble;

		renderData.horizonHeight=windowHeight/2+renderData.cameraPitchScreenAdjustment;

		// Clear z-buffer to infinity values.
		for(unsigned i=0; i<windowWidth*windowHeight; ++i)
			zBuffer[i]=std::numeric_limits<double>::max();

		// Clear surface.
		setRenderDrawColor(colourBg);
		SDL_Rect rect={0, 0, windowWidth, windowHeight};
		SDL_RenderFillRect(renderer, &rect);

		// Draw sky and ground.
		renderSkyGround(renderData);

		// Draw blocks.
		renderBlocks(renderData);

		// Draw object sprites
		renderObjects(renderData);

		// Draw z-buffer
		if (renderData.drawZBuffer)
			renderZBuffer(renderData);
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
		setRenderDrawColor(colourBlack);
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
				setRenderDrawColor(blockInfo.colour);
				SDL_Rect rect={SX(x), SY(y), cellW/divisor, cellH/divisor};
				SDL_RenderFillRect(renderer, &rect);
			}
		}

		// Draw grid over blocks
		setRenderDrawColor(colourGrey);
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
			setRenderDrawColor(colourRed);
			SDL_Rect rect={SX(x)+1, SY(y)+1, (cellW-1)/divisor, (cellH-1)/divisor};
			SDL_RenderFillRect(renderer, &rect);

			// Advance ray.
			ray.next();
		}

		// Draw cross-hair to represent camera
		int k=2;
		setRenderDrawColor(colourBlue);
		SDL_RenderDrawLine(renderer, SX(camera.getX())-k, SY(camera.getY())-k, SX(camera.getX())+k, SY(camera.getY())+k);
		SDL_RenderDrawLine(renderer, SX(camera.getX())-k, SY(camera.getY())+k, SX(camera.getX())+k, SY(camera.getY())-k);

		// Draw camera's line of sight
		setRenderDrawColor(colourGreen);
		double len=10.0;
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
		double distanceFactor=(distance>1.0 ? 1.0/sqrt(distance) : 1.0);
		return brightnessMin+distanceFactor*(brightnessMax-brightnessMin);
	}

	Colour Renderer::colourAdjustForDistance(const Colour &colour, double distance) const {
		Colour temp=colour;
		temp.mul(colourDistanceFactor(distance));
		return temp;
	}

	void Renderer::setRenderDrawColor(const Colour &colour) {
		SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
	}

	void Renderer::renderSkyGround(const RenderData &renderData) {
		// Draw sky and ground as a series of lines varying in colour to simulate distance
		int y;
		for(y=0;y<windowHeight;++y) {
			if (y<renderData.horizonHeight) {
				// Calculate distance in order to adjust colour.
				double distance=unitBlockHeight/(2*(renderData.horizonHeight-y));

				// Sky
				setRenderDrawColor(colourAdjustForDistance(colourSky, distance));
				SDL_RenderDrawLine(renderer, 0, y, windowWidth, y);
			} else {
				// Calculate distance in order to adjust colour.
				double distance=unitBlockHeight/(2*(y-renderData.horizonHeight));

				// Ground
				setRenderDrawColor(colourAdjustForDistance(colourGround, distance));
				SDL_RenderDrawLine(renderer, 0, y, windowWidth, y);
			}
		}
	}

	void Renderer::renderBlocks(const RenderData &renderData) {
		// Loop over each vertical slice of the screen.
		int x;
		for(x=0;x<windowWidth;++x) {
			// Trace ray from view point at this angle to collect a list of 'slices' of blocks to later draw.
			const double deltaAngle=atan((x-windowWidth/2)/renderData.screenDist);
			const double angle=renderData.camera.getYaw()+deltaAngle;
			Ray ray(renderData.camera.getX(), renderData.camera.getY(), angle);

			#define SlicesMax 64
			BlockDisplaySlice slices[SlicesMax];
			size_t slicesNext=0;

			ray.next(); // advance ray to first intersection point
			while(ray.getTrueDistance()<renderData.camera.getMaxDist()) {
				// Get info for block at current ray position.
				if (!getBlockInfoFunctor(ray.getMapX(), ray.getMapY(), &slices[slicesNext].blockInfo, getBlockInfoUserData)) {
					ray.next(); // advance ray here as we skip proper advancing futher in loop body
					continue; // no block
				}

				// We have already added blockInfo to slice stack, so add and compute other fields now.
				slices[slicesNext].distance=ray.getTrueDistance();
				slices[slicesNext].intersectionSide=ray.getSide();
				slices[slicesNext].blockDisplayBase=computeBlockDisplayBase(slices[slicesNext].distance, renderData.cameraZScreenAdjustment, renderData.cameraPitchScreenAdjustment);
				slices[slicesNext].blockDisplayHeight=computeBlockDisplayHeight(slices[slicesNext].blockInfo.height, slices[slicesNext].distance);
				if (slices[slicesNext].blockInfo.texture!=NULL) {
					int textureW=slices[slicesNext].blockInfo.texture->getWidth();
					slices[slicesNext].blockTextureX=ray.getTextureX(textureW);
				}

				// Advance ray to next itersection now ready for next iteration, and for use in block top calculations.
				ray.next();

				// If top of block is visible, compute some extra stuff.
				int blockDisplayTop=slices[slicesNext].blockDisplayBase-slices[slicesNext].blockDisplayHeight;
				if (blockDisplayTop>renderData.horizonHeight) {
					slices[slicesNext].nextDistance=ray.getTrueDistance();
					int nextBlockDisplayBase=computeBlockDisplayBase(slices[slicesNext].nextDistance, renderData.cameraZScreenAdjustment, renderData.cameraPitchScreenAdjustment);
					int nextBlockDisplayHeight=computeBlockDisplayHeight(slices[slicesNext].blockInfo.height, slices[slicesNext].nextDistance);
					int nextBlockDisplayTop=nextBlockDisplayBase-nextBlockDisplayHeight;
					slices[slicesNext].blockDisplayTopSize=blockDisplayTop-nextBlockDisplayTop;
				}

				// If this block occupies whole column already, no point searching further.
				// FIXME: this logic will break if we end up supporting mapping textures with transparency onto blocks
				if (slices[slicesNext].blockDisplayHeight==slices[slicesNext].blockDisplayBase) {
					++slicesNext;
					break;
				}

				// Push slice to stack
				++slicesNext;
			}

			// Loop over found blocks in reverse
			while(slicesNext>0) {
				// Adjust slicesNext now due to how it usually points one beyond last entry
				--slicesNext;

				// Draw block
				if (!renderData.drawZBuffer) {
					if (slices[slicesNext].blockInfo.texture!=NULL) {
						// Textured block
						// Note: we always ask the renderer to draw the whole slice - even if say we are very close to a block and so most of the slice is not visible anyway - hoping that it interally optimises the blit.

						uint8_t colourMod=255;
						if (slices[slicesNext].intersectionSide==Ray::Side::Horizontal)
							colourMod*=0.6;
						colourMod*=colourDistanceFactor(slices[slicesNext].distance);
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
						blockDisplayColour=colourAdjustForDistance(blockDisplayColour, slices[slicesNext].distance);

						// Draw block
						setRenderDrawColor(blockDisplayColour);
						SDL_RenderDrawLine(renderer, x, slices[slicesNext].blockDisplayBase-slices[slicesNext].blockDisplayHeight, x, slices[slicesNext].blockDisplayBase);
					}
				}

				// Update z-buffer
				int zBufferYLoopStart=std::max(0, slices[slicesNext].blockDisplayBase-slices[slicesNext].blockDisplayHeight);
				int zBufferYLoopEnd=std::min(slices[slicesNext].blockDisplayBase, windowHeight-1);
				for(int y=zBufferYLoopStart; y<=zBufferYLoopEnd; ++y) {
					assert(slices[slicesNext].distance<zBuffer[x+y*windowWidth]);
					zBuffer[x+y*windowWidth]=slices[slicesNext].distance;
				}

				// Do we need to draw top of this block? (because it is below the horizon)
				int blockDisplayTop=slices[slicesNext].blockDisplayBase-slices[slicesNext].blockDisplayHeight;
				if (blockDisplayTop>renderData.horizonHeight) {
					// This is a bit more involved because as we move across the slice of the block's top we change in distance from the camera
					int zBufferYLoopStart=blockDisplayTop-slices[slicesNext].blockDisplayTopSize;
					int zBufferYLoopEnd=blockDisplayTop;
					int y1=std::max(0, zBufferYLoopStart);
					int y2=std::min(zBufferYLoopEnd, windowHeight-1);
					for(int y=y1; y<=y2; ++y) {
						// Compute distance and update z-buffer
						assert(slices[slicesNext].distance<=slices[slicesNext].nextDistance);

						double frac=(y-zBufferYLoopStart)/(zBufferYLoopEnd-zBufferYLoopStart+1.0);
						assert(frac>=0.0 && frac<=1.0);

						zBuffer[x+y*windowWidth]=frac*(slices[slicesNext].distance-slices[slicesNext].nextDistance)+slices[slicesNext].nextDistance;

						// Draw pixel
						if (!renderData.drawZBuffer) {
							Colour blockTopDisplayColour=slices[slicesNext].blockInfo.colour;
							blockTopDisplayColour.mul(1.05); // brighten top of blocks as if illuminated from above
							blockTopDisplayColour=colourAdjustForDistance(blockTopDisplayColour, zBuffer[x+y*windowWidth]);
							setRenderDrawColor(blockTopDisplayColour);
							SDL_RenderDrawPoint(renderer, x, y);
						}
					}
				}
			}

			#undef SlicesMax
		}
	}

	void Renderer::renderObjects(const RenderData &renderData) {
		// Grab a list of visible objects and sort them for painting
		std::vector<Object *> *objects=getObjectsInRangeFunctor(renderData.camera, getObjectsInRangeUserData);

		RendererCompareObjectsByDistance compareObjectsByDistance(renderData.camera);
		std::sort(objects->begin(), objects->end(), compareObjectsByDistance); // sort so that we paint closer objects over the top of further away ones (the z buffer is not enough if textures are partially transparent)

		// Loop over and draw all visible objects
		for(auto object : *objects) {
			// Determine angle (and distance) from camera to object, and skip drawing if object is behind camera.
			double objectVisibleAngle, objectBearing, objectDistance;
			renderData.camera.getTargetInfo(object->getCamera(), &objectVisibleAngle, &objectBearing, &objectDistance);

			objectBearing=angleNormalise(objectBearing);
			if (objectBearing<0.5*M_PI || objectBearing>1.5*M_PI)
				continue;

			// Determine x-coordinate of screen where object should appear, and its width.
			// Skip drawing if zero-width or off screen (too far left or right).
			int objectCentreScreenX=tan(objectBearing)*renderData.screenDist+windowWidth/2;
			int objectScreenW=computeBlockDisplayHeight(object->getWidth(), objectDistance);
			if (objectScreenW<=0 || objectCentreScreenX+objectScreenW/2<0 || objectCentreScreenX-objectScreenW/2>=windowWidth)
				continue;

			// Compute base and height of object on screen, and skip drawing if zero-height or off screen (too high/low).
			int objectScreenBase=computeBlockDisplayBase(objectDistance, renderData.cameraZScreenAdjustment, renderData.cameraPitchScreenAdjustment);
			int objectScreenH=computeBlockDisplayHeight(object->getHeight(), objectDistance);
			if (objectScreenH<=0 || objectScreenBase<0 || objectScreenBase-objectScreenH>=windowHeight)
				continue;

			// Grab texture info and compute factors used to map screen pixels to texture pixels.
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
					if (objectDistance>zBuffer[sx+sy*windowWidth])
						continue;

					// Grab pixel from texture and skip if completely transparent.
					int textureExtractX=tx*textureXFactor;
					Colour pixel=objectTexture->getPixel(textureExtractX, textureExtractY);
					if (pixel.a==0)
						continue;

					// Update z-buffer (no need if not drawing it - we already draw objects back-to-front anyway)
					if (renderData.drawZBuffer)
						zBuffer[sx+sy*windowWidth]=objectDistance;

					// Draw pixel
					if (!renderData.drawZBuffer) {
						setRenderDrawColor(colourAdjustForDistance(pixel, objectDistance));
						SDL_RenderDrawPoint(renderer, sx, sy);
					}
				}
			}
		}

		delete objects;
	}

	void Renderer::renderZBuffer(const RenderData &renderData) {
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

};
