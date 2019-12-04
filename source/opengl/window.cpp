#include "window.h"
#include "../core/application.h"
#include "glad/glad.h"
#include <string>

// IMGUI support
#include "../thirdparty/imgui.h"
#include "../thirdparty/imgui_impl_sdl.h"
#include "../thirdparty/imgui_impl_opengl3.h"

extern "C" {
	/*
		Laptops with discrete GPUs tend to auto-select the integrated graphics instead of the
		discrete GPU. (such as Nvidia or AMD)

		These declarations tell the GPU driver to pick the discrete GPU if available.

		https://gist.github.com/statico/6809850727c708f08458
		http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
		http://developer.amd.com/community/blog/2015/10/02/amd-enduro-system-for-developers/
	*/
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;			// Nvidia
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;		// AMD
}

void InitIMGUI(SDL_Window* window, SDL_GLContext gl_context)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	// IO is only used for setting the config flags
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Platform/Renderer bindings
	//SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
	//SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(nullptr);
}

// Called before SDL shuts down
void ShutdownIMGUI()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

OpenGLWindow::OpenGLWindow()
{
	ApplicationSettings settings = GetApplicationSettings();
	Initialize(settings.windowWidth, settings.windowHeight, settings.fullscreen, settings.vsync);
}

OpenGLWindow::OpenGLWindow(int width, int height, bool fullscreenEnabled, bool vsync)
{
	Initialize(width, height, fullscreenEnabled, vsync);
}

OpenGLWindow::~OpenGLWindow()
{
	ShutdownIMGUI();
	if (window)
	{
		SDL_GL_DeleteContext(maincontext);
		SDL_DestroyWindow(window);
	}
}

void OpenGLWindow::SetTitle(std::string newCaption)
{
	SDL_SetWindowTitle(window, newCaption.c_str());
}

void OpenGLWindow::SwapFramebuffer()
{
	SDL_GL_SwapWindow(window);
}

void OpenGLWindow::SetClearColor(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
}

void OpenGLWindow::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void OpenGLWindow::HandleImguiEvent(const SDL_Event* event)
{
	ImGui_ImplSDL2_ProcessEvent(event);
}

void OpenGLWindow::NewImguiFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();
}

void OpenGLWindow::RenderImguiFrame()
{
	ImGui::Render();
	//glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	//glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	//glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void OpenGLWindow::Initialize(int width, int height, bool fullscreen, bool vsync)
{
	auto sdl_die = [](const char* message) {
		fprintf(stderr, "%s: %s\n", message, SDL_GetError());
		exit(2);
	};

	// Initialize SDL 
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		sdl_die("Couldn't initialize SDL");
	}

	atexit(SDL_Quit);
	SDL_GL_LoadLibrary(NULL); // Default OpenGL is fine.

	// Request an OpenGL context (should be core)
	//SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// Create the window
	if (fullscreen)
	{
		window = SDL_CreateWindow(
			"",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL
		);
	}
	else
	{
		window = SDL_CreateWindow(
			"",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			width, height, SDL_WINDOW_OPENGL
		);
	}
	if (window == NULL) sdl_die("Couldn't set video mode");

	maincontext = SDL_GL_CreateContext(window);
	if (maincontext == NULL)
	{
		sdl_die("Failed to create OpenGL context");
	}

	// Check OpenGL properties
	printf("OpenGL loaded\n");
	gladLoadGLLoader(SDL_GL_GetProcAddress);
	printf("Vendor:   %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version:  %s\n", glGetString(GL_VERSION));

	// Use v-sync
	SDL_GL_SetSwapInterval(vsync ? 1 : 0);

	// Disable depth test and face culling.
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	glViewport(0, 0, w, h);
	glScissor(0, 0, w, h);

	InitIMGUI(window, maincontext);
}

