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
	private:
		struct Block {
			double height; // if set to 0.0 then no block here
			Colour colour;
			int textureId; // set to -1 if no texture
		};

		bool hasInit;

		SDL_Renderer *renderer;

		int width, height;

		Block *blocks;
	};
}

#endif
