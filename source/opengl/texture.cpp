#include "texture.h"

#include <iostream>
#include <memory>
#include <algorithm>
#include "../thirdparty/lodepng.h"

#define INTERNAL_PIXEL_FORMAT GL_RGBA
#define PIXEL_FORMAT GL_RGBA
#define PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

void GLTexture::UpdateParameters()
{
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, INTERNAL_PIXEL_FORMAT, width, height, 0, PIXEL_FORMAT, PIXEL_TYPE, (GLvoid*)glData.data());
}

inline void GLTexture::SetPixel(unsigned int pixelIndex, GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
	glData[pixelIndex + 0] = r;
	glData[pixelIndex + 1] = g;
	glData[pixelIndex + 2] = b;
	glData[pixelIndex + 3] = a;
}

void GLTexture::SetPixel(unsigned int x, unsigned int y, GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
	SetPixel(PixelArrayIndex(x, y), r, g, b, a);
}

void GLTexture::SetPixel(unsigned int x, unsigned int y, double r, double g, double b, double a)
{
	r = std::max(std::min(1.0, r), 0.0);
	g = std::max(std::min(1.0, g), 0.0);
	b = std::max(std::min(1.0, b), 0.0);
	a = std::max(std::min(1.0, a), 0.0);
	SetPixel(x, y, GLubyte(r*255.0), GLubyte(g*255.0), GLubyte(b*255.0), GLubyte(a*255.0));
}

void GLTexture::SetPixel(unsigned int x, unsigned int y, Color& color)
{
	unsigned int pixelIndex = PixelArrayIndex(x, y);
	glData[pixelIndex + 0] = color.r;
	glData[pixelIndex + 1] = color.g;
	glData[pixelIndex + 2] = color.b;
	glData[pixelIndex + 3] = color.a;
}

void GLTexture::SetPixel(unsigned int x, unsigned int y, FColor& color)
{
	unsigned int pixelIndex = PixelArrayIndex(x, y);
	glData[pixelIndex + 0] = GLubyte(std::max(std::min(1.0f, color.r), 0.0f) * 255);
	glData[pixelIndex + 1] = GLubyte(std::max(std::min(1.0f, color.g), 0.0f) * 255);
	glData[pixelIndex + 2] = GLubyte(std::max(std::min(1.0f, color.b), 0.0f) * 255);
	glData[pixelIndex + 3] = GLubyte(std::max(std::min(1.0f, color.a), 0.0f) * 255);
}

void GLTexture::SetPixelSafe(int x, int y, Color& color)
{
	if (x > 0 && y > 0 && x < width && y < height)
	{
		SetPixel(x, y, color);
	}
}

void GLTexture::SetPixelSafe(int x, int y, FColor& color)
{
	if (x > 0 && y > 0 && x < width && y < height)
	{
		SetPixel(x, y, color);
	}
}

unsigned int GLTexture::PixelArrayIndex(unsigned int x, unsigned int y)
{
	return y * width * 4 + x * 4;
}

void GLTexture::UseForDrawing()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId);
}

void GLTexture::CopyToGPU()
{
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, PIXEL_FORMAT, PIXEL_TYPE, (GLvoid*)glData.data());
}

void GLTexture::Fill(Color& color)
{
	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			SetPixel(x, y, color);
		}
	}
}

void GLTexture::Fill(FColor& color)
{
	Color remapped;
	remapped.r = GLubyte(std::max(std::min(1.0f, color.r), 0.0f) * 255);
	remapped.g = GLubyte(std::max(std::min(1.0f, color.g), 0.0f) * 255);
	remapped.b = GLubyte(std::max(std::min(1.0f, color.b), 0.0f) * 255);
	remapped.a = GLubyte(std::max(std::min(1.0f, color.a), 0.0f) * 255);

	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			SetPixel(x, y, remapped);
		}
	}
}

void GLTexture::FillDebug()
{
	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			GLubyte r = (GLubyte)(x / (float)width * 255);
			GLubyte g = (GLubyte)(y / (float)height * 255);
			GLubyte b = 0;
			GLubyte a = 255;
			SetPixel(x, y, r, g, b, a);
		}
	}
}

void GLTexture::SaveAsPNG(std::filesystem::path filepath, bool incrementNewFile)
{
	unsigned error = lodepng::encode(filepath.string(), glData, (unsigned int)width, (unsigned int)height);
	if (error)
	{
		std::cout << "encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	}
}

void GLTexture::LoadPNG(std::filesystem::path filepath)
{
	unsigned sourceWidth, sourceHeight;

	glData.clear();
	glData.shrink_to_fit();

	std::vector<unsigned char> png;
	lodepng::State state;
	unsigned error = lodepng::load_file(png, filepath.string());
	if (!error)
	{
		error = lodepng::decode(glData, sourceWidth, sourceHeight, state, png);
	}

	if (error)
	{
		std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	}
	else
	{
		const LodePNGColorMode& color = state.info_png.color;
		//switch (color.colortype)
		//{
		//case LCT_RGB:
		//	PIXEL_FORMAT = GL_RGB;
		//	break;
		//case LCT_RGBA:
		//default:
		//	PIXEL_FORMAT = GL_RGBA;
		//	break;
		//}

		width = sourceWidth;
		height = sourceHeight;

		size = width * height * lodepng_get_channels(&color);

		UpdateParameters();
	}
}
