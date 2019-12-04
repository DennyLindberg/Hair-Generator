#include "program.h"

#include <string>
#include <iostream>

GLUBO::GLUBO()
{
	glGenBuffers(1, &uboId);
}

GLUBO::~GLUBO()
{
	glDeleteBuffers(1, &uboId);
}

void GLUBO::Bind(GLuint point)
{
	glBindBufferBase(GL_UNIFORM_BUFFER, point, uboId);
}

void GLUBO::Allocate(GLuint numbytes)
{
	glBindBuffer(GL_UNIFORM_BUFFER, uboId);
	glBufferData(GL_UNIFORM_BUFFER, numbytes, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	allocated_size = numbytes;
}

GLProgram::GLProgram()
{
	programId = glCreateProgram();
	fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
	vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	// geometry_shader_id created on LoadGeometryShader (as it is optional)
}

GLProgram::~GLProgram()
{
	glDeleteProgram(programId);
	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);

	if (HasGeometryShader())
	{
		glDeleteShader(geometry_shader_id);
	}
}

void GLProgram::LoadFragmentShader(std::string shaderText)
{
	GLint sourceLength = (GLint)shaderText.size();
	const char *fragmentSourcePtr = shaderText.c_str();
	glShaderSource(fragment_shader_id, 1, &fragmentSourcePtr, &sourceLength);
}

void GLProgram::LoadVertexShader(std::string shaderText)
{
	GLint sourceLength = (GLint)shaderText.size();
	const char *vertexSourcePtr = shaderText.c_str();
	glShaderSource(vertex_shader_id, 1, &vertexSourcePtr, &sourceLength);
}

void GLProgram::LoadGeometryShader(std::string shaderText)
{
	if (!HasGeometryShader())
	{
		geometry_shader_id = glCreateShader(GL_GEOMETRY_SHADER);
	}

	GLint sourceLength = (GLint)shaderText.size();
	const char* geometrySourcePtr = shaderText.c_str();
	glShaderSource(geometry_shader_id, 1, &geometrySourcePtr, &sourceLength);
}

GLint CompileAndPrintStatus(GLuint glShaderId)
{
	GLint compileStatus = 0;
	glCompileShader(glShaderId);
	glGetShaderiv(glShaderId, GL_COMPILE_STATUS, &compileStatus);

	if (compileStatus == GL_FALSE)
	{
		std::string message("");

		int infoLogLength = 0;
		glGetShaderiv(glShaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength == 0)
		{
			message = "Message is empty (GL_INFO_LOG_LENGTH == 0)";
		}
		else
		{
			std::unique_ptr<GLchar[]> infoLog(new GLchar[infoLogLength]);
			int charsWritten = 0;
			glGetShaderInfoLog(glShaderId, infoLogLength, &charsWritten, infoLog.get());
			message = std::string(infoLog.get());
		}

		std::cout << L"GL_INFO_LOG: " << message;
	}

	return compileStatus;
}

GLint GLProgram::LinkAndPrintStatus()
{
	glAttachShader(programId, vertex_shader_id);
	glAttachShader(programId, fragment_shader_id);
	if (HasGeometryShader())
	{
		glAttachShader(programId, geometry_shader_id);
	}
	glLinkProgram(programId);

	GLint linkStatus = 0;
	glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);
	if (linkStatus == GL_FALSE)
	{
		std::string message("");

		int infoLogLength = 0;
		std::unique_ptr<GLchar[]> infoLog(new GLchar[infoLogLength]);
		glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength == 0)
		{
			message = "Message is empty (GL_INFO_LOG_LENGTH == 0)";
		}
		else
		{
			std::unique_ptr<GLchar[]> infoLog(new GLchar[infoLogLength]);
			int charsWritten = 0;
			glGetProgramInfoLog(programId, infoLogLength, &charsWritten, infoLog.get());
			message = std::string(infoLog.get());
		}

		std::cout << L"GL_INFO_LOG: " << message;
		return 0;
	} 

	glDetachShader(programId, vertex_shader_id);
	glDetachShader(programId, fragment_shader_id);
	if (HasGeometryShader())
	{
		glDetachShader(programId, geometry_shader_id);
	}

	return linkStatus;
}

void GLProgram::CompileAndLink()
{
	bool VertexShaderCompiled   = CompileAndPrintStatus(vertex_shader_id)   == GL_TRUE;
	bool FragmentShaderCompiled = CompileAndPrintStatus(fragment_shader_id) == GL_TRUE;
	bool GeometryShaderCompiled = true; // optional
	if (HasGeometryShader())
	{
		GeometryShaderCompiled = CompileAndPrintStatus(geometry_shader_id) == GL_TRUE;
	}

	if (!VertexShaderCompiled || !FragmentShaderCompiled || !GeometryShaderCompiled)
	{
		std::cout << L"Failed to compile shaders\n";
	}
	else if (LinkAndPrintStatus() == GL_TRUE)
	{
		// These attributes are bound by default
		glBindAttribLocation(programId, 0, "vertexPosition");
		glBindAttribLocation(programId, 1, "vertexTCoord");

		ReloadUniforms();
	}
}

void GLProgram::Use()
{
	glUseProgram(programId);
}

GLuint GLProgram::Id()
{
	return programId;
}

void GLProgram::SetUniformInt(std::string name, int value)
{
	if (intUniforms.count(name) == 0)
	{
		intUniforms[name] = {
			glGetUniformLocation(programId, name.c_str()),
			value
		};
		intUniforms[name].Upload();
	}
	else
	{
		auto& u = intUniforms[name];
		u.value = value;
		u.Upload();
	}
}

void GLProgram::SetUniformFloat(std::string name, float value)
{
	if (floatUniforms.count(name) == 0)
	{
		floatUniforms[name] = {
			glGetUniformLocation(programId, name.c_str()),
			value
		};
		floatUniforms[name].Upload();
	}
	else
	{
		auto& u = floatUniforms[name];
		u.value = value;
		u.Upload();
	}
}

void GLProgram::SetUniformVec3(std::string name, glm::fvec3 value)
{
	if (vec3Uniforms.count(name) == 0)
	{
		vec3Uniforms[name] = {
			glGetUniformLocation(programId, name.c_str()),
			value
		};
		vec3Uniforms[name].Upload();
	}
	else
	{
		auto& u = vec3Uniforms[name];
		u.value = value;
		u.Upload();
	}
}

void GLProgram::SetUniformVec4(std::string name, glm::fvec4 value)
{
	if (vec4Uniforms.count(name) == 0)
	{
		vec4Uniforms[name] = {
			glGetUniformLocation(programId, name.c_str()),
			value
		};
		vec4Uniforms[name].Upload();
	}
	else
	{
		auto& u = vec4Uniforms[name];
		u.value = value;
		u.Upload();
	}
}

void GLProgram::SetUniformMat4(std::string name, glm::mat4 value)
{
	if (mat4Uniforms.count(name) == 0)
	{
		mat4Uniforms[name] = {
			glGetUniformLocation(programId, name.c_str()),
			value
		};
		mat4Uniforms[name].Upload();
	}
	else
	{
		auto& u = mat4Uniforms[name];
		u.value = value;
		u.Upload();
	}
}

void GLProgram::ReloadUniforms()
{
	Use();

	for (auto& u : intUniforms)
	{
		u.second.id = glGetUniformLocation(programId, u.first.c_str());
		u.second.Upload();
	}

	for (auto& u : floatUniforms)
	{
		u.second.id = glGetUniformLocation(programId, u.first.c_str());
		u.second.Upload();
	}

	for (auto& u : vec3Uniforms)
	{
		u.second.id = glGetUniformLocation(programId, u.first.c_str());
		u.second.Upload();
	}

	for (auto& u : vec4Uniforms)
	{
		u.second.id = glGetUniformLocation(programId, u.first.c_str());
		u.second.Upload();
	}

	for (auto& u : mat4Uniforms)
	{
		u.second.id = glGetUniformLocation(programId, u.first.c_str());
		u.second.Upload();
	}
}
