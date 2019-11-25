#include "shadermanager.h"
#include "../core/utilities.h"

namespace fs = std::filesystem;

void ShaderManager::InitializeFolder(std::filesystem::path shaderFolder)
{
	rootFolder = shaderFolder;
	fileListener.StartThread(shaderFolder);
}

void ShaderManager::LoadLiveShader(GLProgram& targetProgram, std::wstring vertexFilename, std::wstring fragmentFilename, std::wstring geometryFilename)
{
	LoadShader(targetProgram, vertexFilename, fragmentFilename, geometryFilename);

	fileListener.Bind(
		vertexFilename,
		[this, &targetProgram](fs::path filePath) -> void
		{
			this->UpdateShader(targetProgram, filePath, ShaderType::VERTEX);
		}
	);

	fileListener.Bind(
		fragmentFilename,
		[this, &targetProgram](fs::path filePath) -> void
		{
			this->UpdateShader(targetProgram, filePath, ShaderType::FRAGMENT);
		}
	);

	if (geometryFilename != L"")
	{
		fileListener.Bind(
			geometryFilename,
			[this, &targetProgram](fs::path filePath) -> void
			{
				this->UpdateShader(targetProgram, filePath, ShaderType::GEOMETRY);
			}
		);
	}
}

void ShaderManager::LoadShader(GLProgram& targetProgram, std::wstring vertexFilename, std::wstring fragmentFilename, std::wstring geometryFilename)
{
	std::string fragment, vertex, geometry;
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

	bool bShouldLoadGeometryShader = (geometryFilename != L"");
	if (bShouldLoadGeometryShader)
	{
		if (!LoadText(rootFolder/geometryFilename, geometry))
		{
			wprintf(L"\r\nFailed to read shader: %Ls\r\n", geometryFilename.c_str());
			return;
		}
	}

	targetProgram.LoadFragmentShader(fragment);
	targetProgram.LoadVertexShader(vertex);
	if (bShouldLoadGeometryShader)
	{
		targetProgram.LoadGeometryShader(geometry);
	}
	targetProgram.CompileAndLink();
}

void ShaderManager::UpdateShader(GLProgram& targetProgram, fs::path filePath, ShaderType type)
{
	std::string text;
	if (!LoadText(filePath, text))
	{
		wprintf(L"\r\nUpdateShader failed to read file: %Ls\r\n", filePath.c_str());
		return;
	}

	//printf("\r\n=======\r\n%s\r\n=======\r\n\r\n", text.c_str());
	std::wstring MessageType = L"";
	switch (type)
	{
	case ShaderType::VERTEX:
	{
		MessageType = L"Vertex";
		targetProgram.LoadVertexShader(text);
		break;
	}
	case ShaderType::FRAGMENT:
	{
		MessageType = L"Fragment";
		targetProgram.LoadFragmentShader(text);
		break;
	}
	case ShaderType::GEOMETRY:
	{
		MessageType = L"Geometry";
		targetProgram.LoadGeometryShader(text);
		break;
	}
	}
	targetProgram.CompileAndLink();

	wprintf(
		L"\r\n%Ls shader updated: %Ls\r\n", 
		MessageType.c_str(),
		filePath.c_str()
	);
}

void ShaderManager::CheckLiveShaders()
{
	fileListener.ProcessCallbacksOnMainThread();
}
