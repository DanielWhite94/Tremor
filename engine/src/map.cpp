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

		// Allocate textures vector
		textures=new std::vector<Texture *>;

		// Allocate blocks array
		blocks=(Block *)malloc(sizeof(Block)*width*height);
		if (blocks==NULL)
			return;

		// Fill blocks array with height=0 to imply empty
		for(unsigned i=0; i<width*height; ++i)
			blocks[i].height=0.0;

		// Done
		hasInit=true;
	}

	Map::Map(SDL_Renderer *renderer, const char *file): renderer(renderer) {
		hasInit=false;

		// Allocate textures vector
		textures=new std::vector<Texture *>;

		// Set fields to indicate empty map initially
		width=0;
		height=0;
		blocks=NULL;

		// Load map as JSON object
		// TODO: check for failure
		std::ifstream mapStream(file);
		json jsonRoot;
		mapStream >> jsonRoot;

		// Parse JSON data - root 'map' object
		json jsonMap=jsonRoot["map"];
		if (!jsonMap.is_object()) {
			std::cout << "Could not load map: no root map object." << std::endl;
			return;
		}

		if (!jsonMap["width"].is_number() || !jsonMap["height"].is_number()) {
			std::cout << "Could not load map: missing/bad 'width' or 'height' values in root map object." << std::endl;
			return;
		}
		width=jsonMap["width"].get<int>();
		height=jsonMap["height"].get<int>();

		if (width<1 || height<1) {
			std::cout << "Could not load map: bad 'width' or 'height' values (width=" << width << ", height=" << height << ")." << std::endl;
			return;
		}

		// Allocate blocks array
		blocks=(Block *)malloc(sizeof(Block)*width*height);
		if (blocks==NULL) {
			std::cout << "Could not load map: could not allocate blocks array." << std::endl;
			return;
		}

		// Fill blocks array with height=0 to imply empty
		for(unsigned i=0; i<width*height; ++i)
			blocks[i].height=0.0;

		// Parse JSON data - load textures
		json jsonTextures=jsonMap["textures"];
		if (jsonTextures.is_array()) {
			for(auto &entry : jsonTextures.items()) {
				json jsonTexture=entry.value();
				if (!jsonTexture["id"].is_number() || !jsonTexture["file"].is_string()) {
					std::cout << "Warning while loading map: bad texture '" << jsonTexture << "'." << std::endl;
					continue;
				}

				int textureId=jsonTexture["id"].get<int>();
				if (textureId<0) {
					std::cout << "Warning while loading map: bad texture id " << jsonTexture["id"] << "." << std::endl;
					continue;
				}
				std::string textureFile=jsonTexture["file"].get<std::string>();
				if (!addTexture(textureId, textureFile.c_str())) {
					std::cout << "Warning while loading map: could not load texture " << textureId << " at '" << textureFile << "'." << std::endl;
					continue;
				}
			}
		}

		// Parse JSON data - load blocks
		json jsonBlocks=jsonMap["blocks"];
		if (jsonBlocks.is_array()) {
			for(auto &entry : jsonBlocks.items()) {
				json jsonBlock=entry.value();

				if (!jsonBlock["x"].is_number() || !jsonBlock["y"].is_number() || !jsonBlock["height"].is_number() || !jsonBlock["colour"].is_object() || !jsonBlock["colour"]["r"].is_number() || !jsonBlock["colour"]["g"].is_number() || !jsonBlock["colour"]["b"].is_number() || !jsonBlock["colour"]["a"].is_number()) {
					std::cout << "Warning while loading map: bad block '" << jsonBlock << "'." << std::endl;
					continue;
				}

				int blockX=jsonBlock["x"].get<int>();
				int blockY=jsonBlock["y"].get<int>();
				double blockHeight=jsonBlock["height"].get<double>();
				int blockColourR=jsonBlock["colour"]["r"].get<int>();
				int blockColourG=jsonBlock["colour"]["g"].get<int>();
				int blockColourB=jsonBlock["colour"]["b"].get<int>();
				int blockColourA=jsonBlock["colour"]["a"].get<int>();
				if (blockX<0 || blockX>=width || blockY<0 || blockY>=height || blockHeight<=0.0 ||
				    blockColourR<0 || blockColourR>255 ||
				    blockColourG<0 || blockColourG>255 ||
				    blockColourB<0 || blockColourB>255 ||
				    blockColourA<0 || blockColourA>255) {
					std::cout << "Warning while loading map: bad block '" << jsonBlock << "'." << std::endl;
					continue;
				}

				// Update blocks array
				Block *block=&blocks[blockX+blockY*width];
				block->height=blockHeight;
				block->colour.r=blockColourR;
				block->colour.g=blockColourG;
				block->colour.b=blockColourB;
				block->colour.a=blockColourA;
				block->textureId=-1;
				if (jsonBlock["texture"].is_number())
					block->textureId=jsonBlock["texture"].get<int>(); // TODO: check id points to valid texture
			}
		}

		// Parse JSON data - load objects
		// TODO: this

		// Done
		hasInit=true;
	}

	Map::~Map() {
		// Free blocks array
		free(blocks);

		// Free textures vector
		delete textures;
	}

	bool Map::getBlockInfoFunctor(int mapX, int mapY, Renderer::BlockInfo *info) {
		// Outside of map region?
		if (mapX<0 || mapX>=width || mapY<0 || mapY>=height)
			return false;

		// Grab block data
		const Block *block=&blocks[mapX+width*mapY];

		// Empty block?
		if (block->height==0.0)
			return false;

		// Fill block info struct
		info->height=block->height;
		info->colour=block->colour;
		info->texture=getTextureById(block->textureId);

		return true;
	}

	std::vector<Object *> *Map::getObjectsInRangeFunctor(const Camera &camera) {
		std::vector<Object *> *list=new std::vector<Object *>;

		// TODO: this

		return list;
	}

	bool Map::getHasInit(void) {
		return hasInit;
	}

	Texture *Map::getTextureById(int id) {
		if (id<0 || id>=textures->size())
			return NULL;
		return textures->at(id);
	}

	bool Map::addTexture(int id, const char *path) {
		// Does a texture already exist with this id?
		if (getTextureById(id)!=NULL)
			return false;

		// Create and load texture
		Texture *texture=new Texture(renderer, path);
		if (!texture->getHasInit()) {
			delete texture;
			return false;
		}

		// Add to vector
		// TODO: Do textures->reserve instead
		while (id>=textures->size())
			textures->push_back(NULL);
		(*textures)[id]=texture;

		return true;
	}

};
