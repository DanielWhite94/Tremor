#include <assert.h>

#include "colour.h"

const Colour colourBlack   ={.r=  0, .g=  0, .b=  0, .a=255};
const Colour colourWhite   ={.r=255, .g=255, .b=255, .a=255};
const Colour colourGrey    ={.r=196, .g=196, .b=196, .a=255};
const Colour colourRed     ={.r=255, .g=  0, .b=  0, .a=255};
const Colour colourGreen   ={.r=  0, .g=255, .b=  0, .a=255};
const Colour colourBlue    ={.r=  0, .g=  0, .b=255, .a=255};

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
