#include "utilities.h"
#include <fstream>

std::string TimeString(double time)
{
	int seconds = int(time);
	int minutes = seconds / 60;
	int hours = minutes / 60;
	double decimals = time - double(seconds);

	return std::to_string(hours) + "h " + std::to_string(minutes % 60) + "m " + std::to_string(seconds % 60) + "." + std::to_string(int(decimals*10.0)) + "s";
}

std::string FpsString(double deltaTime)
{
	if (deltaTime == 0)
	{
		return "Inf";
	}
	else
	{
		return std::to_string(int(round(1.0 / deltaTime)));
	}
}

bool LoadText(std::filesystem::path filePath, std::string& output)
{
	output = "";
	if (!std::filesystem::exists(filePath)) return false;

	std::ifstream InputFileStream(filePath.c_str());
	if (InputFileStream && InputFileStream.is_open())
	{
		output.assign((std::istreambuf_iterator<char>(InputFileStream)), std::istreambuf_iterator< char >());
		return true;
	}

	return false;
}
