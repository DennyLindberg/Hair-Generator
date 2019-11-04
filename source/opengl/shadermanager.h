#pragma once
#include "program.h"
#include "../core/filelistener.h"
#include <string>
#include <filesystem>

class ShaderManager
{
protected:
	std::filesystem::path rootFolder;
	FileListener fileListener;

public:
	ShaderManager() = default;
	~ShaderManager() = default;

	void InitializeFolder(std::filesystem::path shaderFolder);
	void LoadLiveShader(GLProgram& targetProgram, std::wstring vertexFilename, std::wstring fragmentFilename);
	void LoadShader(GLProgram& targetProgram, std::wstring vertexFilename, std::wstring fragmentFilename);
	void UpdateShader(GLProgram& targetProgram, std::filesystem::path filePath, bool isVertexShader);
	void CheckLiveShaders();
};