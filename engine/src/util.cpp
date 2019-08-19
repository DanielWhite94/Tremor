#ifndef TREMORENGINE_RENDERER_H
#define TREMORENGINE_RENDERER_H

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
};

#endif
