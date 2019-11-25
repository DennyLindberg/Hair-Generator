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
	GLTexture defaultTexture{ textureFolder / "default.png" };
	GLTexture hair_color{ textureFolder / "sparrow_roots.png" };
	GLTexture hair_alpha{ textureFolder / "sparrow_alpha.png" };
	GLTexture hair_id{ textureFolder / "sparrow_id.png" };
	defaultTexture.UseForDrawing();

	// Uniform Buffer Object containing matrices
	GLUBO CameraUBO, LightUBO;
	CameraUBO.Bind(1);
	CameraUBO.Allocate(16 * 8 + 16); // 2 matrices => 8 columns => 16 bytes per column, +vec3 16 bytes
	LightUBO.Bind(2);
	LightUBO.Allocate(16 * 2);

	// Change each LoadShader call to LoadLiveShader for live editing
	GLProgram lineShader, backgroundShader, phongShader, hairShader, bezierLinesShader, shellsShader;
	ShaderManager shaderManager;
	shaderManager.InitializeFolder(shaderFolder);
	shaderManager.LoadShader(lineShader, L"line_vertex.glsl", L"line_fragment.glsl");
	shaderManager.LoadShader(backgroundShader, L"background_vertex.glsl", L"background_fragment.glsl");

	shaderManager.LoadLiveShader(phongShader, L"phong_vertex.glsl", L"phong_fragment.glsl", L"phong_geometry.glsl");
	shaderManager.LoadLiveShader(hairShader, L"bezier_vertex.glsl", L"hair_fragment.glsl", L"linestrip_to_bezier_planes_geometry.glsl");
	shaderManager.LoadLiveShader(bezierLinesShader, L"bezier_vertex.glsl", L"line_fragment.glsl", L"linestrip_to_bezier_lines_geometry.glsl");
	shaderManager.LoadLiveShader(shellsShader, L"shells_vertex.glsl", L"shells_fragment.glsl", L"shells_geometry.glsl");

	// Initialize model values
	glm::mat4 identity_transform{ 1.0f };
	lineShader.Use();
	lineShader.SetUniformMat4("model", identity_transform);
	phongShader.Use(); 
	phongShader.SetUniformMat4("model", identity_transform);
	hairShader.Use();
	hairShader.SetUniformMat4("model", identity_transform);
	bezierLinesShader.Use();
	bezierLinesShader.SetUniformMat4("model", identity_transform);
	shellsShader.Use();
	shellsShader.SetUniformMat4("model", identity_transform);

	// Initialize light source in shaders
	glm::vec4 lightColor{ 1.0f, 1.0f, 1.0f, 1.0f };
	glm::vec3 lightPosition{ 999999.0f };
	LightUBO.SetData(glm::value_ptr(lightPosition), 0, 12);
	LightUBO.SetData(glm::value_ptr(lightColor), 16, 16);

	/*
		Create line strips for testing

		UV-coordinate regions based on sparrow textures
		U							V
		0.0 - 0.2	long/thick		0-1
		0.2 - 0.35	medium/thick	0-1
		0.35 - 0.45 thin1			0-1
		0.47 - 0.6	thin2			0-1
		0.6 - 0.66	thin3			0.1-1

		0.7-1.0		dense/short		0.7-1
		0.7-1.0		dense/short		0.1-0.65
	*/
	GLBezierStrips bezierStrips;
	std::vector<glm::fvec3> bezierStripsPoints1   = {glm::fvec3{0.0f, 0.15f,  0.0f},  glm::fvec3{0.1f, 0.15f, -0.05f}, glm::fvec3{0.2f, 0.15f, -0.4f}};
	std::vector<glm::fvec3> bezierStripsNormals1  = {glm::fvec3{0.0f, 1.0f,  0.0f},  glm::fvec3{0.0f, 1.0f,  0.0f},  glm::fvec3{0.0f, 1.0f,  0.0f}};
	std::vector<glm::fvec3> bezierStripsTangents1 = { 0.2f * (bezierStripsPoints1[1] - bezierStripsPoints1[0]) + glm::fvec3{0.0f, 0.05f,  0.0f}, 0.2f * (bezierStripsPoints1[2] - bezierStripsPoints1[1]) + glm::fvec3{0.0f, -0.05f,  0.0f}, 0.2f * (bezierStripsPoints1[2] - bezierStripsPoints1[1]) };
	std::vector<glm::fvec3> bezierStripsTexcoord1 = { glm::fvec3{0.01f, 0.01f, 0.2f}, glm::fvec3{0.01f, 0.5f, 0.2f}, glm::fvec3{0.01f, 1.0f, 0.2f} };
	std::vector<float> bezierStripsWidths1        = { 0.1f, 0.05f, 0.02f };
	std::vector<float> bezierStripsThickness1     = { 0.1f, 0.05f, 0.01f };
	std::vector<int> bezierStripsSegmentShape1    = { 2, 2, 2 };
	std::vector<int> bezierStripsSegmentSubdivs1  = { 4, 1, 0 }; // last element is pointless

	std::vector<glm::fvec3> bezierStripsPoints2   = {glm::fvec3{0.0f, 0.3f, -0.0f},  glm::fvec3{0.1f, 0.3f, -0.05f}, glm::fvec3{0.2f, 0.3f, -0.4f}};
	std::vector<glm::fvec3> bezierStripsNormals2  = {glm::fvec3{0.0f, 1.0f,  0.0f},  glm::fvec3{0.0f, 1.0f,  0.0f},  glm::fvec3{0.0f, 1.0f,  0.0f}};
	std::vector<glm::fvec3> bezierStripsTangents2 = { 0.2f*(bezierStripsPoints1[1]-bezierStripsPoints1[0]), 0.2f*(bezierStripsPoints1[2]-bezierStripsPoints1[1]), 0.2f*(bezierStripsPoints1[2]-bezierStripsPoints1[1]) };
	std::vector<glm::fvec3> bezierStripsTexcoord2 = { glm::fvec3{0.01f, 0.01f, 0.2f}, glm::fvec3{0.01f, 0.5f, 0.2f}, glm::fvec3{0.01f, 1.0f, 0.2f} };
	std::vector<float> bezierStripsWidths2		  = { 0.1f/2.0f, 0.05f/2.0f, 0.02f/2.0f };
	std::vector<float> bezierStripsThickness2	  = { 0.01f, 0.05f, 0.1f };
	std::vector<int> bezierStripsSegmentShape2    = { 1, 1, 1 };
	std::vector<int> bezierStripsSegmentSubdivs2  = { 1, 4, 0 }; // last element is pointless

	bezierStrips.AddBezierStrip(bezierStripsPoints1, bezierStripsNormals1, bezierStripsTangents1, bezierStripsTexcoord1, bezierStripsWidths1, bezierStripsThickness1, bezierStripsSegmentShape1, bezierStripsSegmentSubdivs1);
	bezierStrips.AddBezierStrip(bezierStripsPoints2, bezierStripsNormals2, bezierStripsTangents2, bezierStripsTexcoord2, bezierStripsWidths2, bezierStripsThickness2, bezierStripsSegmentShape2, bezierStripsSegmentSubdivs2);
	bezierStrips.SendToGPU();

	/*
		Load mesh
	*/
	GLTriangleMesh bunnymesh, malemesh, femalemesh;
	GLMesh::LoadOBJ(meshFolder/"lpshead.obj", malemesh);
	//GLMesh::LoadOBJ(meshFolder/"sparrow.obj", femalemesh);
	//GLMesh::LoadOBJ(meshFolder/"bunny_lowres.obj", bunnymesh);

	femalemesh.transform.position = glm::vec3(0.0f, 0.08f, 0.08f);
	femalemesh.transform.scale = glm::vec3(0.0125f);

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
		shellsShader.Use();
		phongShader.SetUniformMat4("model", malemesh.transform.ModelMatrix());
		malemesh.Draw();
		phongShader.SetUniformMat4("model", femalemesh.transform.ModelMatrix());
		femalemesh.Draw();

		// Line strips
		hairShader.Use();
		hair_color.UseForDrawing(0);
		hair_alpha.UseForDrawing(1);
		hair_id.UseForDrawing(2);
		//bezierStrips.Draw();

		// Grid
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		grid.Draw(projectionmatrix * viewmatrix);
		
		// Clear depth so that we can draw lines on top of everything
		glClear(GL_DEPTH_BUFFER_BIT);

		// Guide lines
		bezierLinesShader.Use();
		//bezierStrips.Draw();
		
		// Coordinate axis'
		lineShader.Use();
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
