#include <cstdio>

#include "map.h"

// Test map - note that texture pointers are filled in later in getBlockInfoFunctor.
#define MapW 16
#define MapH 16
#define _ {.height=0.0, .texture=NULL}
#define w {.height=1.1, .colour={.r=128, .g=128, .b=128}, .texture=NULL}
#define W {.height=0.7, .colour={.r=120, .g=120, .b=120}, .texture=NULL}
#define D {.height=1.4, .colour={.r=120, .g=120, .b=120}, .texture=NULL}
#define T {.height=2.1, .colour={.r=120, .g=120, .b=120}, .texture=NULL}
#define H {.height=2.8, .colour={.r=120, .g=120, .b=120}, .texture=NULL}
#define B {.height=1.5, .colour={.r=235, .g=50, .b=52}, .texture=NULL}
#define s1 {.height=0.1, .colour={.r=177, .g=3, .b=252}, .texture=NULL}
#define s2 {.height=0.2, .colour={.r=177, .g=3, .b=252}, .texture=NULL}
#define s3 {.height=0.3, .colour={.r=177, .g=3, .b=252}, .texture=NULL}
#define s4 {.height=0.4, .colour={.r=177, .g=3, .b=252}, .texture=NULL}
#define s5 {.height=0.5, .colour={.r=177, .g=3, .b=252}, .texture=NULL}
#define s6 {.height=0.6, .colour={.r=177, .g=3, .b=252}, .texture=NULL}
#define s7 {.height=0.7, .colour={.r=177, .g=3, .b=252}, .texture=NULL}
#define s8 {.height=0.8, .colour={.r=177, .g=3, .b=252}, .texture=NULL}
#define s9 {.height=0.9, .colour={.r=177, .g=3, .b=252}, .texture=NULL}
const Renderer::BlockInfo map[MapH][MapW]={
	{ W, W, W, _, _, _, _, _, _, _, _, _, _, _, _, _},
	{ _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _},
	{ w, w, w, _, w, _, B, B, B, _, B, B, B, _, B, B},
	{ w, _, _, _, w, _, _, _, _, _, _, _, _, _, _, B},
	{ w, _, w, _, _, _, _, _, _, _, _, _, _, _, _, _},
	{ w, _, w, _, w, w, w, _, _, _, _, _, _, _, _, B},
	{ _, _, w, _, w, _, _, _, _, _, _, _, _, _, _, B},
	{ w, _, _, _, _, _, _, _, _, _, _, _, _, _, _, B},
	{ w, _, w, _, W, W, W, W, W, W, W, _, _, _, _, _},
	{ w, _, w, _, W, D, D, D, D, D, W, _, _, _, _, B},
	{ _, _, _, _, W, D, T, T, T, D, W, _, _, _, _, B},
	{ _, _, _, _, W, D, T, H, T, D, W, _, _, _, _, B},
	{s3,s2,s1, _, W, D, T, T, T, D, W, _, _, _, _, _},
	{s4,s5,s6, _, W, D, D, D, D, D, W, _, _, _, _, B},
	{s9,s8,s7, _, W, W, W, W, W, W, W, _, _, _, _, B},
	{ _, _, _, _, _, _, _, _, _, _, _, B, B, B, _, B},
};
#undef _
#undef w
#undef D
#undef T
#undef H
#undef s1
#undef s2
#undef s3
#undef s4
#undef s5
#undef s6
#undef s7
#undef s8
#undef s9

bool mapGetBlockInfoFunctor(int mapX, int mapY, Renderer::BlockInfo *info, void *userData) {
	class Map *map=(class Map *)userData;
	return map->getBlockInfoFunctor(mapX, mapY, info);
}

Map::Map(SDL_Renderer *renderer): renderer(renderer) {
	// Load textures
	// TODO: better error handling than exit
	textureWall1=IMG_LoadTexture(renderer, "images/wall1.png");
	if (textureWall1==NULL) {
		printf("Could not created load texture at '%s': %s\n", "images/wall1.png", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	textureWall2=IMG_LoadTexture(renderer, "images/wall2.png");
	if (textureWall2==NULL) {
		printf("Could not created load texture at '%s': %s\n", "images/wall2.png", SDL_GetError());
		exit(EXIT_FAILURE);
	}
}

Map::~Map() {
	SDL_DestroyTexture(textureWall1);
	SDL_DestroyTexture(textureWall2);
}

bool Map::getBlockInfoFunctor(int mapX, int mapY, Renderer::BlockInfo *info) {
	// Outside of pre-defined region?
	if (mapX<0 || mapX>=MapW || mapY<0 || mapY>=MapH) {
		// Create a grid of pillars in half the landscape at least to show distance
		if (!((mapX%5==0) && (mapY%5==0) && mapX>0))
			return false;
		info->height=6.0;
		info->colour.r=64;
		info->colour.g=64;
		info->colour.b=64;
		info->texture=NULL;
		return true;
	}

	// Inside predefined map region - so use array.
	if (map[mapY][mapX].height<0.0001)
		return false;

	*info=map[mapY][mapX];

	// More test map hacks
	if (info->height==1.1)
		info->texture=textureWall1;
	if (info->colour.r==120 && info->colour.g==120 && info->colour.b==120)
		info->texture=textureWall2;

	return true;
}
