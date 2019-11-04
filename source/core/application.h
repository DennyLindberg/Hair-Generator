#pragma once
#include <filesystem>

struct ApplicationSettings
{
	bool vsync = false;
	bool fullscreen = false;
	int windowWidth = 0;
	int windowHeight = 0;
	float windowRatio = 0;
	std::filesystem::path contentPath;
};

void InitializeApplication(ApplicationSettings newInfo);
ApplicationSettings GetApplicationSettings();
void SetThreadedTime(double newTime);
double GetThreadedTime();