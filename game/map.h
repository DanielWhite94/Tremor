#ifndef MAP_H
#define MAP_H

#include <engine.h>

using namespace TremorEngine;

// Wrapper function suitable for passing to Renderer constructor (with class pointer as userData)
bool mapGetBlockInfoFunctor(int mapX, int mapY, Renderer::BlockInfo *info, void *userData);

class Map {
public:
	Map(SDL_Renderer *renderer);
	~Map();

	bool getBlockInfoFunctor(int mapX, int mapY, Renderer::BlockInfo *info);

private:
	SDL_Renderer *renderer;

	SDL_Texture *textureWall1;
	SDL_Texture *textureWall2;

};

#endif
