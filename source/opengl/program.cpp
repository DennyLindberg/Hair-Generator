#include "program.h"

#include <string>
#include <iostream>

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
	geometry_shader_id = glCreateShader(GL_GEOMETRY_SHADER);

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
	else
	{
		glAttachShader(programId, vertex_shader_id);
		glAttachShader(programId, fragment_shader_id);
		if (HasGeometryShader())
		{
			glAttachShader(programId, geometry_shader_id);
		}
		glLinkProgram(programId);

		int infoLogLength = 0;
		glGetProgramiv(programId, GL_LINK_STATUS, &infoLogLength);
		if (infoLogLength > 0) {
			wprintf(L"\r\nSomething went wrong in linkage");
			//fprintf(stderr, "ERROR: could not link shader program GL index %u\n",
			//	shader_program);

			//const int max_length = 2048;
			//int actual_length = 0;
			//char plog[2048];
			//glGetProgramInfoLog(shader_program, max_length, &actual_length, plog);
			//fprintf(stderr, "program info log for GL index %u:\n%s", shader_program, plog);

			//glDeleteProgram(shader_program);
			//return 0;
		}

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

void GLProgram::UpdateMVP(glm::mat4& mvp)
{
	Use();
	glUniformMatrix4fv(mvpId, 1, GL_FALSE, &mvp[0][0]);
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

void GLProgram::ReloadUniforms()
{
	Use();

	mvpId = glGetUniformLocation(programId, "mvp");

	glm::mat4 identity{ 1.0f };
	UpdateMVP(identity);

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
}

