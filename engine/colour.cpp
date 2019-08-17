#include <assert.h>

#include "colour.h"

void Colour::mul(double factor) {
	assert(factor>=0.0);
	if (r*factor>255.0)
		r=255;
	else
		r*=factor;
	if (g*factor>255.0)
		g=255;
	else
		g*=factor;
	if (b*factor>255.0)
		b=255;
	else
		b*=factor;
}
