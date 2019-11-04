#include "grid.h"

glm::mat4 planeRotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3{ 1.0f, 0.0f, 0.0f });

GLGrid::GLGrid()
{
	mesh = std::make_shared<GLQuad>();

	gridShaderProgram.LoadFragmentShader(R"glsl(
			#version 330

			in vec4 TCoord;
			in vec3 position;

			layout(location = 0) out vec4 color;
			uniform float gridSpacing;
			uniform float opacity;
			
			void main() 
			{
				// Antialiased grid, slightly modified
				// http://madebyevan.com/shaders/grid/

				float gridScaling = 0.5f/gridSpacing;
				vec2 lineCoords = position.xy * gridScaling;
				vec2 grid = abs(fract(lineCoords-0.5)-0.5) / fwidth(lineCoords);
				float lineMask = min(1.0, min(grid.x, grid.y));

				color = vec4(lineMask, lineMask, lineMask, lineMask*opacity);
			}
		)glsl");

	gridShaderProgram.LoadVertexShader(R"glsl(
			#version 330

			layout(location = 0) in vec3 vertexPosition;
			layout(location = 1) in vec4 vertexTCoord;
			uniform mat4 mvp;
			uniform float size;

			out vec4 TCoord;
			out vec3 position;

			void main()
			{
				gl_Position = mvp * vec4(vertexPosition*size, 1.0f);
				TCoord = vertexTCoord;
				position = vertexPosition*size;
			}
		)glsl");

	gridShaderProgram.CompileAndLink();
	glBindAttribLocation(gridShaderProgram.Id(), 0, "vertexPosition");
	glBindAttribLocation(gridShaderProgram.Id(), 1, "vertexTCoord");
	mvpUniform = glGetUniformLocation(gridShaderProgram.Id(), "mvp");
	gridUniform = glGetUniformLocation(gridShaderProgram.Id(), "gridSpacing");
	sizeUniform = glGetUniformLocation(gridShaderProgram.Id(), "size");
	opacityUniform = glGetUniformLocation(gridShaderProgram.Id(), "opacity");
}

void GLGrid::Draw(glm::mat4& mvp)
{
	glm::mat4 mvpOffset = mvp * planeRotationMatrix;
	
	gridShaderProgram.Use();
	glUniform1fv(gridUniform, 1, &gridSpacing);
	glUniform1fv(sizeUniform, 1, &size);
	glUniform1fv(opacityUniform, 1, &opacity);
	glUniformMatrix4fv(mvpUniform, 1, GL_FALSE, &mvpOffset[0][0]);
	mesh->Draw();
}