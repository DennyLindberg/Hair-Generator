#define USE_MULTITHREADING false

// STL includes
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

// Application includes
#include "opengl/window.h"
#include "opengl/mesh.h"
#include "opengl/texture.h"
#include "opengl/program.h"
#include "opengl/screenshot.h"
#include "opengl/canvas.h"
#include "core/application.h"
#include "core/clock.h"
#include "core/randomization.h"
#include "core/threads.h"
#include "core/utilities.h"

#include "generation/turtle2d.h"
#include "generation/lsystem.h"
#include "generation/fractals.h"

/*
	Program configurations
*/
static const bool WINDOW_VSYNC = false;
static const int WINDOW_FULLSCREEN = 0;
static const int WINDOW_WIDTH = 640;
static const int WINDOW_HEIGHT = 480;
static const float CAMERA_FOV = 90.0f;
static const float WINDOW_RATIO = WINDOW_WIDTH / float(WINDOW_HEIGHT);
namespace fs = std::filesystem;

/*
	Application
*/
int main()
{
	fs::path contentFolder = fs::current_path().parent_path() / "content";
	InitializeApplication(ApplicationSettings{
		WINDOW_VSYNC, WINDOW_FULLSCREEN, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_RATIO, contentFolder
	});

	UniformRandomGenerator uniformGenerator;
	ApplicationClock clock;

	OpenGLWindow window;
	window.SetTitle("Plant Generation");
	window.SetClearColor(0.0, 0.0, 0.0, 1.0f);

	GLuint defaultVao = 0;
	glGenVertexArrays(1, &defaultVao);
	glBindVertexArray(defaultVao);

	Canvas2D canvas;
	canvas.Fill(Color{255, 255, 255, 255});
	DrawFractalTree(canvas, 6, 3.0f, glm::fvec2(WINDOW_WIDTH * 0.3, WINDOW_HEIGHT), 90);
	DrawKochCurve(canvas, 4, 2.0f, glm::fvec2(0, WINDOW_HEIGHT), 0);
	DrawSierpinskiTriangle(canvas, 5, 8.0f, glm::fvec2(WINDOW_WIDTH, 0), -90);
	DrawDragonCurve(canvas, 11, 3.5f, glm::fvec2(WINDOW_WIDTH * 0.92, WINDOW_HEIGHT*0.8), -90);
	DrawFractalPlant(canvas, 6, 1.75f, glm::fvec2(0, WINDOW_HEIGHT*0.3), 0);

	DrawFractalTreeNezumiV2(canvas, 6, 25.0f, glm::fvec2(WINDOW_WIDTH * 0.55, WINDOW_HEIGHT), 90);
	DrawFractalTreeNezumiV3(canvas, 6, 25.0f, glm::fvec2(WINDOW_WIDTH * 0.55, WINDOW_HEIGHT*0.35), 90);

	std::vector<glm::fvec3> leafHull;
	DrawFractalLeaf(leafHull, canvas, Color{ 0, 0, 0, 255 }, 6, 1.0f, glm::fvec2(WINDOW_WIDTH * 0.55, WINDOW_HEIGHT*0.65), 90);

	double lastScreenUpdate = clock.time;
	bool quit = false;
	int startX = 0;
	int startY = 0;
	while (!quit)
	{
		clock.Tick();
		window.SetTitle("Time: " + TimeString(clock.time) + ", FPS: " + FpsString(clock.deltaTime));

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
			{
				quit = true;
				break;
			}
			case SDL_KEYDOWN:
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					quit = true;
					break;
				case SDLK_s:
					TakeScreenshot("screenshot.png", WINDOW_WIDTH, WINDOW_HEIGHT);
					break;
				}
			}
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEMOTION:
			{
				//SDL_MouseButtonEvent& b = event.button;
				//if (b.button == SDL_BUTTON_LEFT) 
				//{
				//	int mouseWindowX = 0;
				//	int mouseWindowY = 0;
				//	SDL_GetMouseState(&mouseWindowX, &mouseWindowY);

				//	int canvasMouseX = mouseWindowX - 50;
				//	int canvasMouseY = mouseWindowY - 50;
				//	canvas.DrawLine(glm::ivec2{ startX, startY }, glm::ivec2{ canvasMouseX, canvasMouseY }, Color{ 0,0,0,255 });
				//	
				//	startX = canvasMouseX;
				//	startY = canvasMouseY;
				//}
			}
			default:
			{

			}
			}
		}

		window.SetClearColor((sinf(float(clock.time))+1.0f)/2.0f, 0.0, 0.0, 1.0f);
		window.Clear();

		canvas.RenderToScreen();

		window.SwapFramebuffer();
	}

	return 0;
}
