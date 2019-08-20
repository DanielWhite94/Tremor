#ifndef TREMORENGINE_TEXTURE_H
#define TREMORENGINE_TEXTURE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "colour.h"

namespace TremorEngine {

	class Texture {
	public:
		Texture(SDL_Renderer *renderer, const char *file); // check getHasInit after calling
		~Texture();

		bool getHasInit(void) const;

		int getWidth(void) const;
		int getHeight(void) const;
		Colour getPixel(int x, int y) const;

		SDL_Texture *getSdlTexture(void) const;
	private:
		bool hasInit;

		int width, height;

		SDL_Texture *texture;

		Colour *pixels;
	};

};

#endif
