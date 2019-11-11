#pragma once
#include <string>
#include <map>
#include "glad/glad.h"
#include "../core/math.h"

struct UniformFloat
{
	GLint id = 0;
	float value = 0.0f;

	void Upload()
	{
		glUniform1f(id, value);
	}
};

struct UniformVec3
{
	GLint id = 0;
	glm::fvec3 value = glm::fvec3{0.0f};

	void Upload()
	{
		glUniform3fv(id, 1, glm::value_ptr(value));
	}
};

struct UniformVec4
{
	GLint id = 0;
	glm::fvec4 value = glm::fvec4{ 0.0f };

	void Upload()
	{
		glUniform4fv(id, 1, glm::value_ptr(value));
	}
};

class GLUBO
{
protected:
	GLuint uboId = 0;
	GLuint allocated_size = 0;

public:
	GLUBO();
	~GLUBO();

	// Set point to the same binding value as used in the glsl shader
	void Bind(GLuint point);

	void Allocate(GLuint numbytes);

	template<typename T>
	void SetData(T* data, GLuint offset, GLuint numbytes)
	{
		assert((offset+numbytes) <= allocated_size);

		glBindBuffer(GL_UNIFORM_BUFFER, uboId);
		glBufferSubData(GL_UNIFORM_BUFFER, offset, numbytes, data);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	/*
		See this article for a more efficient way of setting up
		an UBO with a struct and memcpy.
		https://www.geeks3d.com/20140704/gpu-buffers-introduction-to-opengl-3-1-uniform-buffers-objects/
	*/
};

class GLProgram
{
protected:
	GLuint programId = 0;
	GLint vertex_shader_id = 0;
	GLint fragment_shader_id = 0;
	GLint geometry_shader_id = -1; // optional

	GLint model_matrix_id = 0;
	std::map<std::string, UniformFloat> floatUniforms;
	std::map<std::string, UniformVec3> vec3Uniforms;
	std::map<std::string, UniformVec4> vec4Uniforms;

public:
	GLProgram();
	~GLProgram();

	bool HasGeometryShader() { return geometry_shader_id != -1; }
	void LoadFragmentShader(std::string shaderText);
	void LoadVertexShader(std::string shaderText);
	void LoadGeometryShader(std::string shaderText);
	GLint LinkAndPrintStatus();
	void CompileAndLink();
	void Use();
	GLuint Id();
	void UpdateModelMatrix(glm::mat4& model);
	void SetUniformFloat(std::string name, float value);
	void SetUniformVec3(std::string name, glm::fvec3 value);
	void SetUniformVec4(std::string name, glm::fvec4 value);
	void ReloadUniforms();
};
