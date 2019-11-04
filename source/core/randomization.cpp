#include "randomization.h"
#include <random>

UniformRandomGenerator::UniformRandomGenerator()
{
	std::random_device rd;
	xorseed[0] = (uint64_t(rd()) << 32) ^ (rd());
	xorseed[1] = (uint64_t(rd()) << 32) ^ (rd());
}

double UniformRandomGenerator::RandomDouble()
{
	return to_double(RandomInt());
}

double UniformRandomGenerator::RandomDouble(double min, double max)
{
	return min + (max - min) * to_double(RandomInt());
}

float UniformRandomGenerator::RandomFloat()
{
	return float(RandomDouble());
}

float UniformRandomGenerator::RandomFloat(float min, float max)
{
	return min + (max - min) * float(RandomDouble());
}