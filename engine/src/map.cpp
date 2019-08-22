#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "map.h"
#include "util.h"

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

		if (jsonRoot.count("map")!=1) {
			std::cout << "Could not load map: no root map object." << std::endl;
			return;
		}
		json jsonMap=jsonRoot["map"];

		// Parse JSON data - metadata (width/height, name etc)
		if (!jsonParseMetadata(jsonMap)) {
			std::cout << "Could not load map: could not read metadata." << std::endl;
			return;
		}

		// Allocate blocks array and fill with height=0 to imply empty
		blocks=(Block *)malloc(sizeof(Block)*width*height);
		if (blocks==NULL) {
			std::cout << "Could not load map: could not allocate blocks array." << std::endl;
			return;
		}

		for(unsigned i=0; i<width*height; ++i)
			blocks[i].height=0.0;

		// Parse JSON data - load textures
		if (jsonMap.count("textures")==1 && jsonMap["textures"].is_array())
			for(auto &entry : jsonMap["textures"].items()) {
				json jsonTexture=entry.value();
				if (!jsonParseTexture(jsonTexture))
					std::cout << "Warning while loading map: bad texture '" << jsonTexture << "'." << std::endl;
			}

		// Parse JSON data - load blocks
		if (jsonMap.count("blocks")==1 && jsonMap["blocks"].is_array()) {
			for(auto &entry : jsonMap["blocks"].items()) {
				json jsonBlock=entry.value();
				if (!jsonParseBlock(jsonBlock))
					std::cout << "Warning while loading map: bad block '" << jsonBlock << "'." << std::endl;
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

	bool Map::jsonParseMetadata(const json &mapObject) {
		// Check map object type.
		if (!mapObject.is_object())
			return false;

		// Parse width and height
		if (mapObject.count("width")!=1 || !mapObject["width"].is_number() || mapObject.count("height")!=1 || !mapObject["height"].is_number()) {
			std::cout << "Could not load metadata: missing/bad 'width' or 'height' values." << std::endl;
			return false;
		}
		width=mapObject["width"].get<int>();
		height=mapObject["height"].get<int>();

		if (width<1 || height<1) {
			std::cout << "Could not load metadata: bad 'width' or 'height' values (width=" << width << ", height=" << height << ")." << std::endl;
			return false;
		}

		// Parse ground and sky colours if given
		if (mapObject.count("groundColour")==1)
			jsonParseColour(mapObject["groundColour"], colourGround);
		if (mapObject.count("skyColour")==1)
			jsonParseColour(mapObject["skyColour"], colourSky);

		// Parse brightness values if given
		if (mapObject.count("brightnessMin")==1 && mapObject["brightnessMin"].is_number()) {
			// TODO: check value is in interval [0.0,1.0]
			brightnessMin=mapObject["brightnessMin"].get<double>();
		}
		if (mapObject.count("brightnessMax")==1 && mapObject["brightnessMax"].is_number()) {
			// TODO: check value is in interval [0.0,1.0]
			brightnessMax=mapObject["brightnessMax"].get<double>();
		}

		// Parse name if given
		if (mapObject.count("name")==1 && mapObject["name"].is_string()) {
			// TODO: Consider sanitising this somewhere along the line
			name=mapObject["name"].get<std::string>();
		}

		return true;
	}

	bool Map::jsonParseTexture(const json &textureObject) {
		if (!textureObject.is_object())
			return false;

		// Grab id
		if (textureObject.count("id")!=1 || !textureObject["id"].is_number())
			return false;
		int textureId=textureObject["id"].get<int>();
		if (textureId<0)
			return false;

		// Grab file
		if (textureObject.count("file")!=1 || !textureObject["file"].is_string())
			return false;
		std::string textureFile=textureObject["file"].get<std::string>();

		// Add texture
		return addTexture(textureId, textureFile.c_str());
	}

	bool Map::jsonParseBlock(const json &blockObject) {
		// Check object is well formed
		if (!blockObject.is_object())
			return false;

		if (blockObject.count("x")!=1 || !blockObject["x"].is_number() ||
		    blockObject.count("y")!=1 || !blockObject["y"].is_number() ||
		    blockObject.count("height")!=1 || !blockObject["height"].is_number())
			return false;

		// Grab block properties
		int blockX=blockObject["x"].get<int>();
		int blockY=blockObject["y"].get<int>();
		double blockHeight=blockObject["height"].get<double>();
		if (blockX<0 || blockX>=width || blockY<0 || blockY>=height || blockHeight<=0.0)
			return false;

		Colour blockColour;
		if (blockObject.count("colour")!=1 || !jsonParseColour(blockObject["colour"], blockColour))
			return false;

		int textureId=-1;
		if (blockObject.count("texture")==1 && blockObject["texture"].is_number()) {
			textureId=blockObject["texture"].get<int>(); // TODO: check id points to valid texture
			if (textureId<0)
				textureId=-1;
		}

		// Update blocks array
		Block *block=&blocks[blockX+blockY*width];
		block->height=blockHeight;
		block->colour=blockColour;
		block->textureId=textureId;

		return true;
	}

	bool Map::jsonParseColour(const json &object, Colour &colour) const {
		// Misssing fields?
		if (!object.is_object() || !object["r"].is_number() || !object["g"].is_number() || !object["b"].is_number())
			return false;

		// Get values
		int r=object["r"].get<int>();
		int g=object["g"].get<int>();
		int b=object["b"].get<int>();
		int a=(object.count("a")==1 && object["a"].is_number() ? object["a"].get<int>() : 255);

		// Clamp values and copy into given colour
		colour.r=clamp(r, 0, 255);
		colour.g=clamp(g, 0, 255);
		colour.b=clamp(b, 0, 255);
		colour.a=clamp(a, 0, 255);

		return true;
	}
};
