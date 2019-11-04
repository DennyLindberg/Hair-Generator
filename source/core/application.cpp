#include "application.h"
#include <atomic>

std::atomic<double> threadedTime = 0.0;
ApplicationSettings info;

void InitializeApplication(ApplicationSettings newInfo)
{
	info = newInfo;
}

ApplicationSettings GetApplicationSettings()
{
	return info;
}

void SetThreadedTime(double newTime)
{
	threadedTime = newTime;
}

double GetThreadedTime()
{
	return threadedTime;
}