#ifndef TREMORENGINE_UTIL_H
#define TREMORENGINE_UTIL_H

namespace TremorEngine {
	typedef long long MicroSeconds;
	static const MicroSeconds microSecondsPerSecond=1000000llu;

	MicroSeconds microSecondsGet(void);
	void microSecondsDelay(MicroSeconds micros);

	double angleNormalise(double angle); // adjusts into interval [0, 2pi)

	int clamp(int x, int a, int b);
};

#endif
