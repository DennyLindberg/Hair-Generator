#include "screenshot.h"

#include <iostream>
#include <fstream>

#include "glad/glad.h"
#include "../thirdparty/lodepng.h"

void TakeScreenshot(std::string filename, unsigned int windowWidth, unsigned int windowHeight)
{
	/*
		Determine file output
	*/
	auto remove_extension = [](const std::string& filename) -> std::string {
		size_t lastdot = filename.find_last_of(".");
		if (lastdot == std::string::npos) return filename;
		return filename.substr(0, lastdot);
	};

	filename = remove_extension(filename);

	auto file_exists = [](std::string filename) -> bool {
		std::ifstream infile(filename);
		return infile.good();
	};

	int count = 0;
	std::string baseName = filename;
	do
	{
		count++;
		filename = baseName + std::string(5 - std::to_string(count).length(), '0') + std::to_string(count) + ".png";
	} while (file_exists(filename));

	/*
		Get data from framebuffer
	*/
	int channelCount = 4;
	std::vector<GLubyte> data(channelCount * windowWidth * windowHeight);
	glReadPixels(0, 0, windowWidth, windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

	// Flip image because OpenGL works from bottom up
	int pixelSourceId = 0;
	int pixelTargetId = 0;
	std::vector<GLubyte> flippedData(4 * windowWidth * windowHeight);
	for (unsigned int x = 0; x < windowWidth; ++x)
	{
		for (unsigned int y = 0; y < windowHeight; ++y)
		{
			pixelSourceId = x*channelCount + y*windowWidth*channelCount;
			pixelTargetId = x*channelCount + (windowHeight-y-1)*windowWidth*channelCount;

			flippedData[pixelTargetId] = data[pixelSourceId];
			flippedData[pixelTargetId+1] = data[pixelSourceId+1];
			flippedData[pixelTargetId+2] = data[pixelSourceId+2];
			flippedData[pixelTargetId+3] = data[pixelSourceId+3];
		}
	}

	/*
		Write PNG
	*/
	unsigned error = lodepng::encode(filename, flippedData, windowWidth, windowHeight);
	if (error)
	{
		std::cout << "\r\nencoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	}
	else
	{
		std::cout << "\r\nScreenshot saved to " + filename + "\r\n";
	}
}
