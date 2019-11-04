#pragma once
#include <vector>
#include "glad/glad.h"
#include "../core/math.h"
#include <filesystem>

class GLTexture
{
public:
	std::vector<GLubyte> glData; // vector is used to simplify load/save with lodepng
	GLuint textureId = 0;
	
	int size = 0;
	int numPixels = 0;
	int width = 0;
	int height = 0;

public:
	GLTexture(std::filesystem::path imagePath)
	{
		LoadPNG(imagePath);
	}

	GLTexture(int textureWidth, int textureHeight)
		: width{ textureWidth }, height{ textureHeight }
	{
		numPixels = width * height;
		size = numPixels * 4;
		glData.resize(size);

		for (int i = 0; i < size; ++i)
		{
			glData[i] = 0;
		}

		glGenTextures(1, &textureId);
		UpdateParameters();
	}

	~GLTexture()
	{
		glDeleteTextures(1, &textureId);
	}

	void UpdateParameters();

	inline GLubyte& operator[] (unsigned int i) { return glData[i]; }

	inline void SetPixel(unsigned int pixelIndex, GLubyte r, GLubyte g, GLubyte b, GLubyte a);
	void SetPixel(unsigned int x, unsigned int y, GLubyte r, GLubyte g, GLubyte b, GLubyte a);
	void SetPixel(unsigned int x, unsigned int y, double r, double g, double b, double a);

	void SetPixel(unsigned int x, unsigned int y, Color& color);
	void SetPixel(unsigned int x, unsigned int y, FColor& color);

	void SetPixelSafe(int x, int y, Color& color);
	void SetPixelSafe(int x, int y, FColor& color);

	unsigned int PixelArrayIndex(unsigned int x, unsigned int y);

	void UseForDrawing();
	void CopyToGPU();

	void Fill(Color& color);
	void Fill(FColor& color);

	void FillDebug();
	void SaveAsPNG(std::filesystem::path filepath, bool incrementNewFile = false);
	void LoadPNG(std::filesystem::path filepath);
};
