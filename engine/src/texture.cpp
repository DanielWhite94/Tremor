#include <cassert>
#include <cstdlib>

#include "texture.h"

namespace TremorEngine {
	Texture::Texture(SDL_Renderer *renderer, const char *path) {
		// Set fields to indicate not initialised
		hasInit=false;
		texture=NULL;

		// Load surface and texture
		SDL_Surface *surface=IMG_Load(path);
		if (surface==NULL)
			return;
		texture=SDL_CreateTextureFromSurface(renderer, surface);
		if (texture==NULL) {
			SDL_FreeSurface(surface);
			return;
		}

		// Query texture properties
		SDL_QueryTexture(texture, NULL, NULL, &width, &height);

		// Allocate pixels array
		pixels=(Colour *)malloc(sizeof(Colour)*width*height);
		if (pixels==NULL) {
			SDL_FreeSurface(surface);
			return;
		}

		// Fill pixels array
		SDL_LockSurface(surface);

		Colour *destPixelPtr=pixels;
		uint8_t *srcPixelPtr=(uint8_t *)surface->pixels;
		for(unsigned i=0; i<height; ++i)
			for(unsigned j=0; j<width; ++j) {
				uint32_t pixel;
				memcpy(&pixel, srcPixelPtr, surface->format->BytesPerPixel);

				SDL_GetRGBA(pixel, surface->format, &(destPixelPtr->r), &(destPixelPtr->g), &(destPixelPtr->b), &(destPixelPtr->a));

				srcPixelPtr+=surface->format->BytesPerPixel;
				++destPixelPtr;
			}

		SDL_UnlockSurface(surface);

		// Tidy up
		SDL_FreeSurface(surface);

		// Done
		hasInit=true;
	}

	Texture::~Texture() {
		if (!hasInit)
			return;

		SDL_DestroyTexture(texture);
		free(pixels);
	}

	bool Texture::getHasInit(void) const {
		return hasInit;
	}

	int Texture::getWidth(void) const {
		return width;
	}

	int Texture::getHeight(void) const {
		return height;
	}

	Colour Texture::getPixel(int x, int y) const {
		assert(x>=0 && x<getWidth());
		assert(y>=0 && y<getHeight());

		return pixels[x+y*getWidth()];
	}

	SDL_Texture *Texture::getSdlTexture(void) const {
		return texture;
	}
};
