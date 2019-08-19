#ifndef TREMORENGINE_RENDERER_H
#define TREMORENGINE_RENDERER_H

#include <cmath>
#include <time.h>

#include "util.h"

namespace TremorEngine {
	MicroSeconds microSecondsGet(void) {
		struct timespec tp;
		clock_gettime(CLOCK_MONOTONIC_RAW, &tp); // TODO: Check return.
		return ((MicroSeconds)tp.tv_sec)*microSecondsPerSecond+tp.tv_nsec/1000;
	}

	void microSecondsDelay(MicroSeconds micros) {
		struct timespec tp;
		tp.tv_sec=micros/microSecondsPerSecond;
		tp.tv_nsec=(micros%microSecondsPerSecond)*1000;
		nanosleep(&tp, NULL);
	}

	double angleNormalise(double angle) {
		angle=fmod(angle,2.0*M_PI);
		if (angle<0.0)
			angle+=2.0*M_PI;
		return angle;
	}
};

#endif
