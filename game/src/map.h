#ifndef MAP_H
#define MAP_H

#include <vector>

#include <engine.h>

using namespace TremorEngine;

// Wrapper functions suitable for passing to Renderer constructor (with class pointer as userData)
bool mapGetBlockInfoFunctor(int mapX, int mapY, Renderer::BlockInfo *info, void *userData);
std::vector<Object *> *mapGetObjectsInRangeFunctor(const Camera &camera, void *userData);

class Map {
public:
	Map(SDL_Renderer *renderer);
	~Map();

	bool getBlockInfoFunctor(int mapX, int mapY, Renderer::BlockInfo *info);
	std::vector<Object *> *getObjectsInRangeFunctor(const Camera &camera);

private:
	SDL_Renderer *renderer;

	SDL_Texture *textureWall1;
	SDL_Texture *textureWall2;
	SDL_Texture *textureBarrel;
	SDL_Texture *textureLois[8];

	Object *objectBarrel;
	Object *objectLois;

};

#endif
