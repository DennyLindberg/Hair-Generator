#include "shadermanager.h"
#include "../core/utilities.h"

namespace fs = std::filesystem;

void ShaderManager::InitializeFolder(std::filesystem::path shaderFolder)
{
	rootFolder = shaderFolder;
	fileListener.StartThread(shaderFolder);
}

void ShaderManager::LoadLiveShader(GLProgram& targetProgram, std::wstring vertexFilename, std::wstring fragmentFilename)
{
	LoadShader(targetProgram, vertexFilename, fragmentFilename);

	bool isVertexShader = true;
	fileListener.Bind(
		vertexFilename,
		[this, &targetProgram, isVertexShader](fs::path filePath) -> void
		{
			this->UpdateShader(targetProgram, filePath, isVertexShader);
		}
	);

	isVertexShader = false;
	fileListener.Bind(
		fragmentFilename,
		[this, &targetProgram, isVertexShader](fs::path filePath) -> void
		{
			this->UpdateShader(targetProgram, filePath, isVertexShader);
		}
	);
}

void ShaderManager::LoadShader(GLProgram& targetProgram, std::wstring vertexFilename, std::wstring fragmentFilename)
{
	std::string fragment, vertex;
	if (!LoadText(rootFolder/vertexFilename, vertex))
	{
		wprintf(L"\r\nFailed to read shader: %Ls\r\n", vertexFilename.c_str());
		return;
	}

	if (!LoadText(rootFolder/fragmentFilename, fragment))
	{
		wprintf(L"\r\nFailed to read shader: %Ls\r\n", fragmentFilename.c_str());
		return;
	}


	targetProgram.LoadFragmentShader(fragment);
	targetProgram.LoadVertexShader(vertex);
	targetProgram.CompileAndLink();
}

void ShaderManager::UpdateShader(GLProgram& targetProgram, fs::path filePath, bool isVertexShader)
{
	wprintf(
		L"\r\n%Ls shader updated: %Ls\r\n", 
		(isVertexShader? L"Vertex" : L"Fragment"),
		filePath.c_str()
	);

	std::string text;
	if (!LoadText(filePath, text))
	{
		wprintf(L"\r\nFailed to read file: %Ls\r\n", filePath.c_str());
		return;
	}

	printf("\r\n=======\r\n%s\r\n=======\r\n\r\n", text.c_str());
	if (isVertexShader)
	{
		targetProgram.LoadVertexShader(text);
	}
	else
	{
		targetProgram.LoadFragmentShader(text);
	}
	targetProgram.CompileAndLink();
}

void ShaderManager::CheckLiveShaders()
{
	fileListener.ProcessCallbacksOnMainThread();
}
