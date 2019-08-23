#ifndef TREMORENGINE_MAP_H
#define TREMORENGINE_MAP_H

#include <string>
#include <vector>

#include "camera.h"
#include "colour.h"
#include "json.hpp"
#include "object.h"
#include "renderer.h"
#include "texture.h"

using json=nlohmann::json;

namespace TremorEngine {
	// Wrapper functions suitable for passing to Renderer constructor (with class pointer as userData)
	bool mapGetBlockInfoFunctor(int mapX, int mapY, Renderer::BlockInfo *info, void *userData);
	std::vector<Object *> *mapGetObjectsInRangeFunctor(const Camera &camera, void *userData);

	class Map {
	public:
		// renderer can be NULL in constructor, but then textures will always fail to add
		Map(SDL_Renderer *renderer, int width, int height);
		Map(SDL_Renderer *renderer, const char *file);
		~Map();

		bool getBlockInfoFunctor(int mapX, int mapY, Renderer::BlockInfo *info);
		std::vector<Object *> *getObjectsInRangeFunctor(const Camera &camera);

		bool getHasInit(void);
		Texture *getTextureById(int id);
		const std::string getName(void) const;
		int getWidth(void) const;
		int getHeight(void) const;
		const Colour &getGroundColour(void) const;
		const Colour &getSkyColour(void) const;
		double getBrightnessMin(void) const;
		double getBrightnessMax(void) const;

		bool addTexture(int id, const char *path);
	private:
		struct Block {
			double height; // if set to 0.0 then no block here
			Colour colour;
			int textureId; // set to -1 if no texture
		};

		bool hasInit;

		SDL_Renderer *renderer;

		std::string name;
		int width, height;
		Colour colourGround, colourSky;
		double brightnessMin, brightnessMax;

		std::vector<Texture *> *textures;
		Block *blocks;
		std::vector<Object *> *objects;

		bool jsonParseMetadata(const json &mapObject);
		bool jsonParseTexture(const json &textureObject);
		bool jsonParseBlock(const json &blockObject);
		bool jsonParseObject(const json &objectObject);

		bool jsonParseColour(const json &object, Colour &colour) const; // colour unchanged if fails
		bool jsonParseCamera(const json &object, Camera &camera) const; // camera unchanged if fails
	};
}

#endif
