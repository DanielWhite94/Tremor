#ifndef TREMORENGINE_COLOUR_H
#define TREMORENGINE_COLOUR_H

#include <cstdint>

class Colour {
public:
	uint8_t r, g, b, a;

	void mul(double factor); // does not affect alpha
};

#endif
