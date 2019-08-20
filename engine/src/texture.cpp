#include "texture.h"

namespace TremorEngine {
	Texture::Texture(SDL_Renderer *renderer, const char *path) {
		hasInit=false;

		texture=IMG_LoadTexture(renderer, path);
		if (texture==NULL)
			return;

		SDL_QueryTexture(texture, NULL, NULL, &width, &height);

		hasInit=true;
	}

	Texture::~Texture() {
		if (!hasInit)
			return;

		SDL_DestroyTexture(texture);
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

	SDL_Texture *Texture::getSdlTexture(void) const {
		return texture;
	}
};
