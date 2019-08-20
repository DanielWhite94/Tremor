#ifndef TREMORENGINE_MAP_H
#define TREMORENGINE_MAP_H

#include <vector>

#include "camera.h"
#include "object.h"
#include "renderer.h"
#include "texture.h"

namespace TremorEngine {
	// Wrapper functions suitable for passing to Renderer constructor (with class pointer as userData)
	bool mapGetBlockInfoFunctor(int mapX, int mapY, Renderer::BlockInfo *info, void *userData);
	std::vector<Object *> *mapGetObjectsInRangeFunctor(const Camera &camera, void *userData);

	class Map {
	public:
		Map(SDL_Renderer *renderer, int width, int height);
		Map(SDL_Renderer *renderer, const char *file);
		~Map();

		bool getBlockInfoFunctor(int mapX, int mapY, Renderer::BlockInfo *info);
		std::vector<Object *> *getObjectsInRangeFunctor(const Camera &camera);

		bool getHasInit(void);
		Texture *getTextureById(int id);
		int getWidth(void) const;
		int getHeight(void) const;

		bool addTexture(int id, const char *path);
	private:
		struct Block {
			double height; // if set to 0.0 then no block here
			Colour colour;
			int textureId; // set to -1 if no texture
		};

		bool hasInit;

		SDL_Renderer *renderer;

		int width, height;

		std::vector<Texture *> *textures;
		Block *blocks;
		std::vector<Object *> *objects;
	};
}

#endif
