#pragma once

#include "SDL2/SDL.h"
#undef main

#include <string>
#include <functional>
#include "../thirdparty/imgui.h"

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

	void HandleImguiEvent(const SDL_Event* event);
	void NewImguiFrame();
	void RenderImguiFrame();

	void OnImguiUpdate(std::function<void()> callback)
	{
		NewImguiFrame();
		callback();
		RenderImguiFrame();
	}

protected:
	void Initialize(int width, int height, bool fullscreenEnabled, bool vsync);
};
