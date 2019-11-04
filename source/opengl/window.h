#pragma once

#include "SDL2/SDL.h"
#undef main

#include <string>

class OpenGLWindow
{
protected:
	SDL_GLContext maincontext;
	SDL_Window* window = nullptr;

public:
	OpenGLWindow();
	OpenGLWindow(int width, int height, bool fullscreenEnabled, bool vsync);

	~OpenGLWindow();

	void SetTitle(std::string newCaption);
	void SwapFramebuffer();
	void SetClearColor(float r, float g, float b, float a);
	void Clear();

protected:
	void Initialize(int width, int height, bool fullscreenEnabled, bool vsync);
};
