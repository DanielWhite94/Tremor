#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "json.hpp"
#include "map.h"

using json=nlohmann::json;

namespace TremorEngine {
	bool mapGetBlockInfoFunctor(int mapX, int mapY, Renderer::BlockInfo *info, void *userData) {
		class Map *map=(class Map *)userData;
		return map->getBlockInfoFunctor(mapX, mapY, info);
	}

	std::vector<Object *> *mapGetObjectsInRangeFunctor(const Camera &camera, void *userData) {
		class Map *map=(class Map *)userData;
		return map->getObjectsInRangeFunctor(camera);
	}

	Map::Map(SDL_Renderer *renderer, int width, int height): renderer(renderer), width(width), height(height) {
		hasInit=false;

		// Allocate blocks array
		blocks=(Block *)malloc(sizeof(Block)*width*height);

		// Fill blocks array with height=0 to imply empty
		for(unsigned i=0; i<width*height; ++i)
			blocks[i].height=0.0;

		// Done
		hasInit=true;
	}

	Map::Map(SDL_Renderer *renderer, const char *file): renderer(renderer) {
		hasInit=false;
		// TODO: this
		width=0;
		height=0;
		blocks=NULL;
	}

	Map::~Map() {
		// Free blocks array
		free(blocks);
	}

	bool Map::getBlockInfoFunctor(int mapX, int mapY, Renderer::BlockInfo *info) {
		// Outside of map region?
		if (mapX<0 || mapX>=width || mapY<0 || mapY>=height)
			return false;

		// Grab block info
		// TODO: this
		return false;
	}

	std::vector<Object *> *Map::getObjectsInRangeFunctor(const Camera &camera) {
		std::vector<Object *> *list=new std::vector<Object *>;

		// TODO: this

		return list;
	}

	bool Map::getHasInit(void) {
		return hasInit;
	}

};
