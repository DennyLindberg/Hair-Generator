// STL includes
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <functional>

// Application includes
#include "opengl/window.h"
#include "opengl/camera.h"
#include "opengl/mesh.h"
#include "opengl/texture.h"
#include "opengl/program.h"
#include "opengl/screenshot.h"
#include "opengl/grid.h"
#include "opengl/canvas.h"
#include "opengl/shadermanager.h"
#include "core/application.h"
#include "core/clock.h"
#include "core/randomization.h"
#include "core/threads.h"
#include "core/utilities.h"
#include "core/input.h"

/*
	Program configurations
*/
static const bool WINDOW_VSYNC = true;
static const int WINDOW_FULLSCREEN = 0;
static const int WINDOW_WIDTH = 1280;
static const int WINDOW_HEIGHT = 720;
static const float CAMERA_FOV = 45.0f;
static const float WINDOW_RATIO = WINDOW_WIDTH / float(WINDOW_HEIGHT);
static const int FPS_LIMIT = 0;

namespace fs = std::filesystem;

/*
	Application
*/
int main()
{
	fs::path contentFolder = fs::current_path().parent_path() / "content";
	fs::path textureFolder = fs::current_path().parent_path() / "content" / "textures";
	fs::path shaderFolder = fs::current_path().parent_path() / "content" / "shaders";
	fs::path meshFolder = fs::current_path().parent_path() / "content" / "meshes";
	InitializeApplication(ApplicationSettings{
		WINDOW_VSYNC, WINDOW_FULLSCREEN, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_RATIO, contentFolder
	});

	UniformRandomGenerator uniformGenerator;
	ApplicationClock clock;

	OpenGLWindow window;
	window.SetTitle("Hair Generator");
	window.SetClearColor(0.5f, 0.5f, 0.5f, 1.0f);

	GLuint defaultVao = 0;
	glGenVertexArrays(1, &defaultVao);
	glBindVertexArray(defaultVao);

printf(R"(
====================================================================
	
    Hair Generator.

    Controls:
        Control the camera
        Alt + LMB: Rotate
        Alt + MMB: Move
        Alt + RMB: Zoom

        F:         Re-center camera on origin

        4:         Display wireframe surfaces
        5:         Display textured surfaces

        6:         Toggle display of skeleton
			       
        S:         Take screenshot
			       
        ESC:       Close the application

====================================================================
)");

	/*
		Setup scene and controls
	*/
	Camera camera;
	camera.fieldOfView = CAMERA_FOV;

	GLQuad backgroundQuad;
	GLGrid grid;
	grid.size = 5.0f;
	grid.gridSpacing = 0.1f;

	TurntableController turntable(camera);
	turntable.position = glm::vec3{0.0f, 0.15f, 0.0f};
	turntable.sensitivity = 0.25f;
	turntable.Set(-65.0f, 15.0f, 1.0f);


	/*
		Load and initialize shaders
	*/
	GLTexture defaultTexture{textureFolder / "default.png"};
	defaultTexture.UseForDrawing();

	// Uniform Buffer Object containing matrices
	glm::mat4 identity_transform{ 1.0f };
	GLUBO CameraUBO, LightUBO;
	CameraUBO.Bind(1);
	CameraUBO.Allocate(16 * 8 + 16); // 2 matrices => 8 columns => 16 bytes per column, +vec3 16 bytes
	LightUBO.Bind(2);
	LightUBO.Allocate(16 * 2);

	// Change each LoadShader call to LoadLiveShader for live editing
	GLProgram lineShader, backgroundShader, geometryShader, linestripToPlanesShader;
	ShaderManager shaderManager;
	shaderManager.InitializeFolder(shaderFolder);
	shaderManager.LoadShader(lineShader, L"line_vertex.glsl", L"line_fragment.glsl");
	shaderManager.LoadShader(backgroundShader, L"background_vertex.glsl", L"background_fragment.glsl");

	shaderManager.LoadLiveShader(geometryShader, L"phong_vertex.glsl", L"phong_fragment.glsl", L"phong_geometry.glsl");
	shaderManager.LoadLiveShader(linestripToPlanesShader, L"line_vertex.glsl", L"phong_fragment.glsl", L"linestrip_to_plane_geometry.glsl");

	// Initialize light source in shaders
	glm::vec4 lightColor{ 1.0f, 1.0f, 1.0f, 1.0f };
	glm::vec3 lightPosition{ 999999.0f };
	LightUBO.SetData(glm::value_ptr(lightPosition), 0, 12);
	LightUBO.SetData(glm::value_ptr(lightColor), 16, 16);

	/*
		Create line strips for testing
	*/
	GLLineStrips lineStrips;
	std::vector<glm::fvec3> lineStripsPoints1 = {
		glm::fvec3{0.0f, 0.0f, -0.0f},
		glm::fvec3{0.1f, 0.0f, -0.05f},
		glm::fvec3{0.2f, 0.0f, -0.4f},
		glm::fvec3{0.3f, 0.0f, -0.6f}
	};
	std::vector<glm::fvec3> lineStripsPoints2 = {
		glm::fvec3{0.0f, 0.1f, -0.0f},
		glm::fvec3{0.1f, 0.1f, -0.05f},
		glm::fvec3{0.2f, 0.1f, -0.4f},
		glm::fvec3{0.3f, 0.1f, -0.6f}
	};
	std::vector<glm::fvec3> lineStripsPoints3 = {
		glm::fvec3{0.0f, 0.2f, -0.0f},
		glm::fvec3{0.1f, 0.2f, -0.05f},
		glm::fvec3{0.2f, 0.2f, -0.4f},
		glm::fvec3{0.3f, 0.2f, -0.6f}
	};
	lineStrips.AddLineStrip(lineStripsPoints1);
	lineStrips.AddLineStrip(lineStripsPoints2);
	lineStrips.AddLineStrip(lineStripsPoints3);
	lineStrips.SendToGPU();

	/*
		Load mesh
	*/
	GLTriangleMesh dummymesh;
	GLMesh::LoadOBJ(meshFolder/"lpshead.obj", dummymesh);
	//GLMesh::LoadOBJ(meshFolder/"bunny_lowres.obj", dummymesh);

	/*
		Coordinate Axis Lines
	*/
	GLLine hierarchyAxisLines, coordinateReferenceLines;
	GLMesh::AppendCoordinateAxis(
		coordinateReferenceLines, 
		glm::fvec3{ 0.0f, 0.0f, 0.0f }, 
		glm::fvec3{ 1.0f, 0.0f, 0.0f }, 
		glm::fvec3{ 0.0f, 1.0f, 0.0f }, 
		glm::fvec3{ 0.0f, 0.0f, 1.0f },
		0.1f
	);
	coordinateReferenceLines.SendToGPU();

	/*
		User interaction options
	*/
	bool renderWireframe = false;
	bool renderTransformHierarchy = false;

	/*
		Main application loop
	*/
	bool quit = false;
	bool captureMouse = false;
	double lastUpdate = 0.0;
	double deltaTime = 0.0;
	double fpsDelta = (FPS_LIMIT == 0) ? 0.0 : (1.0 / FPS_LIMIT);
	while (!quit)
	{
		clock.Tick();
		SetThreadedTime(clock.time);

		if (WINDOW_VSYNC || FPS_LIMIT == 0)
		{
			deltaTime = clock.deltaTime;
			lastUpdate = clock.time;
		}
		else
		{
			deltaTime = clock.time - lastUpdate;
			if (deltaTime < fpsDelta) continue;
			lastUpdate = clock.time;
		}

		window.SetTitle("FPS: " + FpsString(deltaTime));
		shaderManager.CheckLiveShaders();

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			quit = (event.type == SDL_QUIT) || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE);
			if (quit) break;

			SDL_Keymod mod = SDL_GetModState();
			bool bCtrlModifier = mod & KMOD_CTRL;
			bool bShiftModifier = mod & KMOD_SHIFT;
			bool bAltModifier = mod & KMOD_ALT;

			if (event.type == SDL_KEYDOWN)
			{
				auto key = event.key.keysym.sym;

				if		(key == SDLK_4) renderWireframe = true;
				else if (key == SDLK_5) renderWireframe = false;
				else if (key == SDLK_6) renderTransformHierarchy = !renderTransformHierarchy;
				else if (key == SDLK_s) TakeScreenshot("screenshot.png", WINDOW_WIDTH, WINDOW_HEIGHT);
				else if (key == SDLK_f) turntable.SnapToOrigin();

				switch (key)
				{
				case SDLK_g:case SDLK_t:case SDLK_UP:case SDLK_DOWN:case SDLK_LEFT:case SDLK_RIGHT:
				{
					//GenerateRandomTree(treeStyle, treeIterations, treeSubdivisions);
				}
				default: { break; }
				}
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN)
			{
				if (bAltModifier)
				{
					captureMouse = true;
					SDL_ShowCursor(0);
					SDL_SetRelativeMouseMode(SDL_TRUE);

					auto button = event.button.button;
						 if (button == SDL_BUTTON_LEFT)   turntable.inputState = TurntableInputState::Rotate;
					else if (button == SDL_BUTTON_MIDDLE) turntable.inputState = TurntableInputState::Translate;
					else if (button == SDL_BUTTON_RIGHT)  turntable.inputState = TurntableInputState::Zoom;

					turntable.OnBeginInput();
				}
			}
			else if (event.type == SDL_MOUSEBUTTONUP)
			{
				captureMouse = false;
				SDL_ShowCursor(1);
				SDL_SetRelativeMouseMode(SDL_FALSE);
			}
			else if (event.type == SDL_MOUSEMOTION && captureMouse)
			{
				turntable.ApplyMouseInput(-event.motion.xrel, event.motion.yrel);
			}
		}

		/*
			Render scene
		*/
		window.Clear();

		// Background color gradient
		backgroundShader.Use();
		backgroundQuad.Draw();
		glClear(GL_DEPTH_BUFFER_BIT);
		
		// Determine scene render properties
		glPolygonMode(GL_FRONT_AND_BACK, (renderWireframe? GL_LINE : GL_FILL));
		glm::mat4 viewmatrix = camera.ViewMatrix();
		glm::mat4 projectionmatrix = camera.ProjectionMatrix();
		CameraUBO.SetData(glm::value_ptr(projectionmatrix), 0, 64);
		CameraUBO.SetData(glm::value_ptr(viewmatrix), 64, 64);
		CameraUBO.SetData(glm::value_ptr(camera.GetPosition()), 128, 16);

		// Render mesh
		geometryShader.Use();
		geometryShader.SetUniformVec3("cameraPosition", camera.GetPosition()); // todo: expand UBO
		//dummymesh.transform.rotation.y = lastUpdate*360.0f;
		geometryShader.UpdateModelMatrix(dummymesh.transform.ModelMatrix()); // todo: replace with SetMatrix4x4
		dummymesh.Draw();

		// Line strips
		linestripToPlanesShader.Use();
		defaultTexture.UseForDrawing();

		geometryShader.SetUniformVec3("cameraPosition", camera.GetPosition()); // todo: expand UBO
		linestripToPlanesShader.UpdateModelMatrix(identity_transform); // todo: replace with SetMatrix4x4
		linestripToPlanesShader.SetUniformFloat("useUniformColor", true);
		linestripToPlanesShader.SetUniformFloat("transformVerticesInVertexShader", false);
		linestripToPlanesShader.SetUniformVec4("uniformColor", glm::fvec4{ 0.0f, 1.0f, 0.0f, 1.0f });
		lineStrips.Draw();

		// Grid
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		grid.Draw(projectionmatrix * viewmatrix);
		
		// Skeleton and coordinate axes
		glClear(GL_DEPTH_BUFFER_BIT);
		lineShader.Use();
		lineShader.UpdateModelMatrix(identity_transform);
		lineShader.SetUniformFloat("useUniformColor", false);
		coordinateReferenceLines.Draw();
		if (renderTransformHierarchy)
		{
			hierarchyAxisLines.Clear();
			MeshTransform a;
			MeshTransform b;
			MeshTransform c;

			a.position = { 1.0f, 0.0f, 0.0f };
			b.position = { 0.0f, 1.0f, 0.0f };
			b.rotation = { 45.0f, 0.0f, 0.0f };
			c.position = { 0.0f, 1.0f, 0.0f };
			c.rotation = { -45.0f, 0.0f, 45.0f };
			
			GLMesh::AppendCoordinateAxis(hierarchyAxisLines, a.ModelMatrix(), 0.1f);
			GLMesh::AppendCoordinateAxis(hierarchyAxisLines, a.ModelMatrix() * b.ModelMatrix(), 0.1f);
			GLMesh::AppendCoordinateAxis(hierarchyAxisLines, a.ModelMatrix() * b.ModelMatrix() * c.ModelMatrix(), 0.1f);
			hierarchyAxisLines.SendToGPU();

			hierarchyAxisLines.Draw();
		}

		// Done
		window.SwapFramebuffer();
	}

	exit(0);
}
