#pragma once
#include <stdint.h>

/*
	Xorshift
	https://stackoverflow.com/questions/35358501/what-is-performance-wise-the-best-way-to-generate-random-bools
*/

class UniformRandomGenerator
{
protected:
	uint64_t xorseed[2] = { 0,0 };

public:
	UniformRandomGenerator();
	~UniformRandomGenerator() = default;

protected:
	static inline double to_double(uint64_t x) 
	{
		const union 
		{ 
			uint64_t i; 
			double d; 
		} u = { 0x3FFull << 52 | x >> 12 };

		return u.d - 1.0;
	}

	// xorshift128plus
	inline uint64_t RandomInt()
	{
		uint64_t x = xorseed[0];
		uint64_t const y = xorseed[1];
		xorseed[0] = y;
		x ^= x << 23; // a
		xorseed[1] = x ^ y ^ (x >> 17) ^ (y >> 26); // b, c
		return xorseed[1] + y;
	}

public:
	double RandomDouble();
	double RandomDouble(double min, double max);
	float RandomFloat();
	float RandomFloat(float min, float max);
};