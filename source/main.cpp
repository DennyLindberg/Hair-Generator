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
	fs::path curvesFolder = fs::current_path().parent_path() / "content" / "curves";
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

	FileListener fileListener;
	fileListener.StartThread(curvesFolder);

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
        7:         Toggle light follow camera
        8:         Toggle hair flat
			       
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
	GLTexture scalp{ textureFolder / "scalp.png" };
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
	GLBezierStrips longHairMesh;
	GLMesh::LoadCurves(curvesFolder / "longhair.json", longHairMesh);
	fileListener.Bind(L"longhair.json", [&longHairMesh](fs::path filePath) -> void 
		{
			GLMesh::LoadCurves(filePath, longHairMesh);
			longHairMesh.SendToGPU();
		}
	);



	/*
		Load mesh
	*/
	GLTriangleMesh bunnymesh, malemesh, femalemesh;
	//GLMesh::LoadOBJ(meshFolder/"lpshead.obj", malemesh);
	GLMesh::LoadOBJ(meshFolder/"sparrow.obj", femalemesh);
	//GLMesh::LoadOBJ(meshFolder/"bunny_lowres.obj", bunnymesh);

	femalemesh.transform.position = glm::vec3(0.0f, 0.08f, 0.08f);
	femalemesh.transform.scale = glm::vec3(0.0125f);
	longHairMesh.transform.position = femalemesh.transform.position;
	longHairMesh.transform.scale = femalemesh.transform.scale;

	/*
		Coordinate Axis Lines
	*/
	GLLine coordinateReferenceLines;
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
	bool renderHead = true;
	bool renderHair = true;
	bool renderWireframe = false;
	bool renderBezierLines = false;
	bool lightFollowsCamera = false;
	bool renderHairFlat = false;
	bool drawDebugNormals = false;
	float hairUnifiedNormalBlend = 0.9f;
	float hairMaskCutoff = 0.25f;
	glm::fvec3 unifiedNormalsCapsuleStart = glm::fvec3(0.0f, 0.0f, 0.05f);
	glm::fvec3 unifiedNormalsCapsuleEnd = glm::fvec3(0.0f, 0.3f, 0.05f);
	glm::fvec3 hairDarkColor = glm::fvec3(33.0f/255.0f, 17.0f/255.0f, 4.0f/255.0f);
	glm::fvec3 hairLightColor = glm::fvec3(145.0f/255.0f, 123.0f/255.0f, 104.0f/255.0f)*0.7f;

	int RenderHairMesh = 0;
	int shapeOverride = -1;
	int subdivisionsOverride = -1;

	/*
		IMGUI callback
	*/
	auto DrawMainUI = [&]() -> void {
		ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH * 0.25f, WINDOW_HEIGHT));
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::Begin("Settings");
		{
			ImGui::Text("Scene");
			ImGui::Checkbox("Render head", &renderHead);
			ImGui::Checkbox("Render hair", &renderHair);
			ImGui::Checkbox("Wireframe", &renderWireframe);
			ImGui::Checkbox("Light follows camera", &lightFollowsCamera);
			ImGui::Text("Hair");
			ImGui::ColorEdit3("Dark Color", (float*)& hairDarkColor);
			ImGui::ColorEdit3("Light Color", (float*)& hairLightColor);
			ImGui::SliderFloat("Mask cutoff", (float*)& hairMaskCutoff, 0.0f, 1.0f);
			ImGui::Checkbox("Debug bezier", &renderBezierLines);
			ImGui::Checkbox("Flat color", &renderHairFlat);
			ImGui::Text("Hair Normals Capsule");
			ImGui::SliderFloat3("Top", (float*)& unifiedNormalsCapsuleEnd, 0.0f, 0.5f);
			ImGui::SliderFloat3("Bottom", (float*)& unifiedNormalsCapsuleStart, 0.0f, 0.5f);
			ImGui::SliderFloat("Amount", &hairUnifiedNormalBlend, 0.0f, 1.0f);
			ImGui::Checkbox("Draw debug normals", &drawDebugNormals);
			ImGui::Text("Hair Overrides");
			ImGui::SliderInt("Shape", &shapeOverride, -1, 2);
			ImGui::SliderInt("Subdivisions", &subdivisionsOverride, -1, 4);


		}
		ImGui::End();
	};

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
		fileListener.ProcessCallbacksOnMainThread();

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			quit = (event.type == SDL_QUIT) || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE);
			if (quit) break;

			window.HandleImguiEvent(&event);

			SDL_Keymod mod = SDL_GetModState();
			bool bCtrlModifier = mod & KMOD_CTRL;
			bool bShiftModifier = mod & KMOD_SHIFT;
			bool bAltModifier = mod & KMOD_ALT;

			if (event.type == SDL_KEYDOWN)
			{
				auto key = event.key.keysym.sym;

				if      (key == SDLK_s) TakeScreenshot("screenshot.png", WINDOW_WIDTH, WINDOW_HEIGHT);
				else if (key == SDLK_f) turntable.SnapToOrigin();
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

		// Update light
		LightUBO.SetData(glm::value_ptr(lightFollowsCamera? camera.GetPosition() : lightPosition), 0, 12);

		if (renderHead)
		{
			phongShader.Use();
			phongShader.SetUniformMat4("model", malemesh.transform.ModelMatrix());
			malemesh.Draw();
			phongShader.SetUniformMat4("model", femalemesh.transform.ModelMatrix());
			femalemesh.Draw();
		}

		if (renderHair)
		{
			hairShader.Use();
			hair_color.UseForDrawing(0);
			hair_alpha.UseForDrawing(1);
			hair_id.UseForDrawing(2);
			hairShader.SetUniformMat4("model", longHairMesh.transform.ModelMatrix());
			hairShader.SetUniformInt("bRenderHairFlat", renderHairFlat);
			hairShader.SetUniformInt("bDrawDebugNormals", drawDebugNormals);
			hairShader.SetUniformVec3("unifiedNormalsCapsuleStart", unifiedNormalsCapsuleStart);
			hairShader.SetUniformVec3("unifiedNormalsCapsuleEnd", unifiedNormalsCapsuleEnd);
			hairShader.SetUniformVec3("darkColor", hairDarkColor);
			hairShader.SetUniformVec3("lightColor", hairLightColor);
			hairShader.SetUniformFloat("normalBlend", hairUnifiedNormalBlend);
			hairShader.SetUniformFloat("maskCutoff", hairMaskCutoff);
			hairShader.SetUniformInt("shapeOverride", shapeOverride);
			hairShader.SetUniformInt("subdivisionsOverride", subdivisionsOverride);
			longHairMesh.Draw();
		}

		// Grid
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		grid.Draw(projectionmatrix * viewmatrix);
		
		// Clear depth so that we can draw lines on top of everything
		glClear(GL_DEPTH_BUFFER_BIT);
		
		// Coordinate axis'
		lineShader.Use();
		lineShader.SetUniformFloat("useUniformColor", false);
		coordinateReferenceLines.Draw();

		if (renderBezierLines)
		{
			bezierLinesShader.Use();
			bezierLinesShader.SetUniformMat4("model", longHairMesh.transform.ModelMatrix());
			bezierLinesShader.SetUniformInt("subdivisionsOverride", subdivisionsOverride);
			longHairMesh.Draw();
		}

		// Done
		window.OnImguiUpdate(DrawMainUI);
		window.SwapFramebuffer();
	}

	exit(0);
}
