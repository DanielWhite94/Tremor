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

		// Set default field values
		name=std::string("Unnamed Map");
		blocks=NULL;
		colourGround.r=0;
		colourGround.g=255;
		colourGround.b=0;
		colourGround.a=255;
		colourSky.r=0;
		colourSky.g=0;
		colourSky.b=255;
		colourSky.a=255;
		brightnessMin=0.0;
		brightnessMax=0.0;

		// Allocate textures vector
		textures=new std::vector<Texture *>;

		// Allocate objects vector
		objects=new std::vector<Object *>;

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

		// Allocate objects vector
		objects=new std::vector<Object *>;

		// Set fields to indicate empty map initially
		name=std::string("Unnamed Map");
		width=0;
		height=0;
		blocks=NULL;
		colourGround.r=0;
		colourGround.g=255;
		colourGround.b=0;
		colourGround.a=255;
		colourSky.r=0;
		colourSky.g=0;
		colourSky.b=255;
		colourSky.a=255;
		brightnessMin=0.0;
		brightnessMax=0.0;

		// Load map as JSON object
		// TODO: check for failure
		std::ifstream mapStream(file);
		json jsonRoot;
		mapStream >> jsonRoot;

		// Parse JSON data - width and height
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

		// Parse JSON data - load ground and sky colours if given
		if (jsonMap["groundColour"].is_object()) {
			// TODO: warning if r, g, b or a are not numbers or bad values.
			if (jsonMap["groundColour"]["r"].is_number() && jsonMap["groundColour"]["g"].is_number() && jsonMap["groundColour"]["b"].is_number() && jsonMap["groundColour"]["a"].is_number()) {
				colourGround.r=jsonMap["groundColour"]["r"].get<int>();
				colourGround.g=jsonMap["groundColour"]["g"].get<int>();
				colourGround.b=jsonMap["groundColour"]["b"].get<int>();
				colourGround.a=jsonMap["groundColour"]["a"].get<int>();
			}
		}
		if (jsonMap["skyColour"].is_object()) {
			// TODO: warning if r, g, b or a are not numbers or bad values.
			if (jsonMap["skyColour"]["r"].is_number() && jsonMap["skyColour"]["g"].is_number() && jsonMap["skyColour"]["b"].is_number() && jsonMap["skyColour"]["a"].is_number()) {
				colourSky.r=jsonMap["skyColour"]["r"].get<int>();
				colourSky.g=jsonMap["skyColour"]["g"].get<int>();
				colourSky.b=jsonMap["skyColour"]["b"].get<int>();
				colourSky.a=jsonMap["skyColour"]["a"].get<int>();
			}
		}

		// Parse JSON data - load brightness values if given
		if (jsonMap["brightnessMin"].is_number()) {
			// TODO: check value is in interval [0.0,1.0]
			brightnessMin=jsonMap["brightnessMin"].get<double>();
		}
		if (jsonMap["brightnessMax"].is_number()) {
			// TODO: check value is in interval [0.0,1.0]
			brightnessMax=jsonMap["brightnessMax"].get<double>();
		}

		// Parse JSON data - load name
		if (jsonMap["name"].is_string()) {
			// TODO: Consider sanitising this somewhere along the line
			name=jsonMap["name"].get<std::string>();
		}

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
		json jsonObjects=jsonMap["objects"];
		if (jsonObjects.is_array()) {
			for(auto &objectEntry : jsonObjects.items()) {
				json jsonObject=objectEntry.value();

				if (!jsonObject["width"].is_number() || !jsonObject["height"].is_number() || !jsonObject["camera"].is_object() || !jsonObject["camera"]["x"].is_number() || !jsonObject["camera"]["y"].is_number() || !jsonObject["camera"]["z"].is_number() || !jsonObject["camera"]["yaw"].is_number() || !jsonObject["textures"].is_array()) {
					std::cout << "Warning while loading map: bad object '" << jsonObject << "'." << std::endl;
					continue;
				}

				double objectWidth=jsonObject["width"].get<double>();
				double objectHeight=jsonObject["height"].get<double>();
				double objectCameraX=jsonObject["camera"]["x"].get<double>();
				double objectCameraY=jsonObject["camera"]["y"].get<double>();
				double objectCameraZ=jsonObject["camera"]["z"].get<double>();
				double objectCameraYaw=jsonObject["camera"]["yaw"].get<double>();

				if (objectWidth<=0.0 || objectHeight<=0.0) {
					std::cout << "Warning while loading map: bad object '" << jsonObject << "'." << std::endl;
					continue;
				}

				Camera objectCamera(objectCameraX, objectCameraY, objectCameraZ, objectCameraYaw);

				Object::MovementParameters objectMovementParameters;
				json jsonMovementParameters=jsonObject["movementParameters"];
				if (jsonMovementParameters.is_object()) {
					if (jsonMovementParameters["standHeight"].is_number())
						objectMovementParameters.standHeight=jsonMovementParameters["standHeight"].get<double>();
				}

				Object *object=new Object(objectWidth, objectHeight, objectCamera, objectMovementParameters);

				for(auto &textureEntry : jsonObject["textures"].items()) {
					json jsonObjectTexture=textureEntry.value();
					if (!jsonObjectTexture.is_number())
						continue;

					int textureId=jsonObjectTexture.get<int>();

					Texture *texture=getTextureById(textureId);
					if (texture==NULL)
						continue;

					object->addTexture(texture);
				}

				objects->push_back(object);
			}
		}

		// Done
		hasInit=true;
	}

	Map::~Map() {
		// Free blocks array
		free(blocks);

		// Free objects vector
		// TODO: delete all entries also?
		delete objects;

		// Free textures vector
		// TODO: delete all entries also?
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

		// TODO: make this more efficient by using a better algorithm and culling based on given camera.
		// For now we simply copy the whole object list each time.
		*list=*objects;

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

	const std::string Map::getName(void) const {
		return name;
	}

	int Map::getWidth(void) const {
		return width;
	}

	int Map::getHeight(void) const {
		return height;
	}

	const Colour &Map::getGroundColour(void) const {
		return colourGround;
	}

	const Colour &Map::getSkyColour(void) const {
		return colourSky;
	}

	double Map::getBrightnessMin(void) const {
		return brightnessMin;
	}

	double Map::getBrightnessMax(void) const {
		return brightnessMax;
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
