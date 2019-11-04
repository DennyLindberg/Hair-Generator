#pragma once
#include "mesh.h"
#include "program.h"
#include <memory>

class GLGrid : public GLQuad
{
protected:
	GLProgram gridShaderProgram;
	std::shared_ptr<GLQuad> mesh;

	GLuint mvpUniform = 0;
	GLuint gridUniform = 0;
	GLuint sizeUniform = 0;
	GLuint opacityUniform = 0;

public:
	float size = 1.0f;
	float gridSpacing = 0.5f;
	float opacity = 0.2f;

	GLGrid();
	~GLGrid() = default;

	void Draw(glm::mat4& mvp);
};