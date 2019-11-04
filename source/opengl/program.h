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

class GLProgram
{
protected:
	GLuint programId = 0;
	GLint vertex_shader_id = 0;
	GLint fragment_shader_id = 0;

	GLint mvpId = 0;
	std::map<std::string, UniformFloat> floatUniforms;
	std::map<std::string, UniformVec3> vec3Uniforms;
	std::map<std::string, UniformVec4> vec4Uniforms;

public:
	GLProgram();
	~GLProgram();

	void LoadFragmentShader(std::string shaderText);
	void LoadVertexShader(std::string shaderText);
	void CompileAndLink();
	void Use();
	GLuint Id();
	void UpdateMVP(glm::mat4& mvp);
	void SetUniformFloat(std::string name, float value);
	void SetUniformVec3(std::string name, glm::fvec3 value);
	void SetUniformVec4(std::string name, glm::fvec4 value);
	void ReloadUniforms();
};
