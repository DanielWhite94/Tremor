#ifndef TREMORENGINE_COLOUR_H
#define TREMORENGINE_COLOUR_H

#include <cstdint>

class Colour {
public:
	uint8_t r, g, b;

	void mul(double factor);
};

#endif
