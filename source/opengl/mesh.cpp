#include "mesh.h"
#include "../core/application.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/euler_angles.hpp"

#include <string>
#include <iostream>

const GLuint positionAttribId = 0;
const GLuint normalAttribId = 1;
const GLuint colorAttribId = 2;
const GLuint texCoordAttribId = 3;

glm::mat4 MeshTransform::ModelMatrix()
{
	glm::mat4 s = glm::scale(glm::mat4{ 1.0f }, scale);
	glm::mat4 r = glm::eulerAngleYXZ(
		glm::radians(rotation.y),
		glm::radians(rotation.x),
		glm::radians(rotation.z)
	);
	glm::mat4 t = glm::translate(glm::mat4{ 1.0f }, position);
	return t * r * s;
}


GLMeshInterface::GLMeshInterface()
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
}

GLMeshInterface::~GLMeshInterface()
{
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);
}




GLTriangleMesh::GLTriangleMesh(bool allocate)
{
	allocated = allocate;
	if (!allocated) return;

	glBindVertexArray(vao);

	glGenBuffers(1, &positionBuffer);
	glGenBuffers(1, &normalBuffer);
	glGenBuffers(1, &colorBuffer);
	glGenBuffers(1, &texCoordBuffer);
	glGenBuffers(1, &indexBuffer);

	// Position buffer
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glEnableVertexAttribArray(positionAttribId);
	glVertexAttribPointer(positionAttribId, 3, GL_FLOAT, false, 0, 0);

	// Normal buffer
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glEnableVertexAttribArray(normalAttribId);
	glVertexAttribPointer(normalAttribId, 3, GL_FLOAT, false, 0, 0);

	// Color buffer
	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
	glEnableVertexAttribArray(colorAttribId);
	glVertexAttribPointer(colorAttribId, 4, GL_FLOAT, false, 0, 0);

	// TexCoord buffer
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
	glEnableVertexAttribArray(texCoordAttribId);
	glVertexAttribPointer(texCoordAttribId, 4, GL_FLOAT, false, 0, 0);

	// Index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
}

GLTriangleMesh::~GLTriangleMesh()
{
	if (!allocated) return;

	glDeleteBuffers(1, &positionBuffer);
	glDeleteBuffers(1, &normalBuffer);
	glDeleteBuffers(1, &colorBuffer);
	glDeleteBuffers(1, &texCoordBuffer);
	glDeleteBuffers(1, &indexBuffer);
}

void GLTriangleMesh::Clear()
{
	positions.clear();
	normals.clear();
	colors.clear();
	texCoords.clear();
	indices.clear();

	positions.shrink_to_fit();
	normals.shrink_to_fit();
	colors.shrink_to_fit();
	texCoords.shrink_to_fit();
	indices.shrink_to_fit();

	SendToGPU();
}

void GLTriangleMesh::SendToGPU()
{
	if (!allocated) return;

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glBufferVector(GL_ARRAY_BUFFER, positions, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferVector(GL_ARRAY_BUFFER, normals, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
	glBufferVector(GL_ARRAY_BUFFER, colors, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
	glBufferVector(GL_ARRAY_BUFFER, texCoords, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferVector(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW);
}

void GLTriangleMesh::Draw()
{
	if (allocated && positions.size() > 0 && indices.size() > 0)
	{
		glBindVertexArray(vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
		glDrawElements(GL_TRIANGLES, GLsizei(indices.size()), GL_UNSIGNED_INT, (void*)0);
	}
}

void GLTriangleMesh::AddVertex(glm::fvec3 pos, glm::fvec4 color, glm::fvec4 texcoord)
{
	positions.push_back(std::move(pos));
	normals.push_back(glm::fvec3{ 0.0f });
	colors.push_back(std::move(color));
	texCoords.push_back(std::move(texcoord));
}

void GLTriangleMesh::AddVertex(glm::fvec3 pos, glm::fvec3 normal, glm::fvec4 color, glm::fvec4 texcoord)
{
	positions.push_back(std::move(pos));
	normals.push_back(std::move(normal));
	colors.push_back(std::move(color));
	texCoords.push_back(std::move(texcoord));
}

void GLTriangleMesh::DefineNewTriangle(unsigned int index1, unsigned int index2, unsigned int index3)
{
	indices.push_back(index1);
	indices.push_back(index2);
	indices.push_back(index3);
}

void GLTriangleMesh::AppendMesh(const GLTriangleMesh& other)
{
	// These two values are used to re-calculate the indices of the other mesh
	// after it has been appended.
	int newIndicesOffset = int(positions.size());
	int newIndicesStart = int(indices.size());

	positions.reserve(positions.size() + other.positions.size());
	normals.reserve(normals.size() + other.normals.size());
	colors.reserve(colors.size() + other.colors.size());
	texCoords.reserve(texCoords.size() + other.texCoords.size());
	indices.reserve(indices.size() + other.indices.size());

	positions.insert(positions.end(), other.positions.begin(), other.positions.end());
	normals.insert(normals.end(), other.normals.begin(), other.normals.end());
	colors.insert(colors.end(), other.colors.begin(), other.colors.end());
	texCoords.insert(texCoords.end(), other.texCoords.begin(), other.texCoords.end());
	indices.insert(indices.end(), other.indices.begin(), other.indices.end());

	for (int i = newIndicesStart; i < indices.size(); ++i)
	{
		indices[i] += newIndicesOffset;
	}
}

void GLTriangleMesh::AppendMeshTransformed(const GLTriangleMesh & other, glm::mat4 transform)
{
	int newPositionsStart = int(positions.size());
	AppendMesh(other);
	int newPositionsEnd = int(positions.size() - 1);
	ApplyMatrix(transform, newPositionsStart, newPositionsEnd);
}

void GLTriangleMesh::ApplyMatrix(glm::mat4 transform, int firstIndex, int lastIndex)
{
	firstIndex = (firstIndex < 0)? 0 : firstIndex;
	lastIndex = (lastIndex >= positions.size()) ? int(positions.size() - 1) : lastIndex;

	for (int i=firstIndex; i<=lastIndex; i++)
	{
		positions[i] = transform * glm::fvec4(positions[i], 1.0f);
		normals[i] = transform * glm::fvec4(normals[i], 0.0f);
	}
}

void GLTriangleMesh::ApplyMatrix(glm::mat4 transform)
{
	ApplyMatrix(transform, 0, int(positions.size() - 1));
}




GLLine::GLLine()
{
	// Generate buffers
	glGenBuffers(1, &positionBuffer);
	glGenBuffers(1, &colorBuffer);

	// Load positions
	int valuesPerPosition = 3; // glm::fvec3 has 3 floats
	glEnableVertexAttribArray(positionAttribId);
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glVertexAttribPointer(positionAttribId, valuesPerPosition, GL_FLOAT, false, 0, 0);

	// Load colors
	valuesPerPosition = 4; // glm::fvec4 has 4 floats
	glEnableVertexAttribArray(colorAttribId);
	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
	glVertexAttribPointer(colorAttribId, valuesPerPosition, GL_FLOAT, false, 0, 0);

	SendToGPU();
}

GLLine::~GLLine()
{
	glDeleteBuffers(1, &positionBuffer);
	glDeleteBuffers(1, &colorBuffer);
}

void GLLine::AddLine(glm::fvec3 start, glm::fvec3 end, glm::fvec4 color)
{
	lineSegments.push_back(GLLineSegment{ std::move(start), std::move(end) });
	colors.push_back(color);
	colors.push_back(std::move(color));
}

void GLLine::Clear()
{
	lineSegments.clear();
	lineSegments.shrink_to_fit();
	colors.clear();
	colors.shrink_to_fit();
	SendToGPU();
}

void GLLine::SendToGPU()
{
	glBindVertexArray(vao);

	// Positions
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glBufferVector(GL_ARRAY_BUFFER, lineSegments, GL_STATIC_DRAW);

	// Colors
	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
	glBufferVector(GL_ARRAY_BUFFER, colors, GL_STATIC_DRAW);
}

void GLLine::Draw()
{
	if (lineSegments.size() > 0)
	{
		glBindVertexArray(vao);
		glDrawArrays(GL_LINES, 0, GLsizei(lineSegments.size()) * 2 * 3);
	}
}





void GLQuadProperties::MatchWindowDimensions()
{
	ApplicationSettings settings = GetApplicationSettings();
	positionX = 0.0f;
	positionY = 0.0f;
	width = float(settings.windowWidth);
	height = float(settings.windowHeight);
}

GLQuad::GLQuad()
{
	CreateMeshBuffer(MeshBufferProperties{
		-1.0f,
		1.0f,
		1.0f,
		-1.0f
	});
}

GLQuad::GLQuad(GLQuadProperties properties)
{
	ApplicationSettings settings = GetApplicationSettings();
	float windowWidth = float(settings.windowWidth);
	float windowHeight = float(settings.windowHeight);

	// GL coordinate system has the origin in the middle of the screen and
	// ranges between -1.0 to 1.0. UI coordinates must be remapped.
	float relativeWidth  = properties.width     / windowWidth;
	float relativeHeight = properties.height    / windowHeight;
	float relativeX      = properties.positionX / windowWidth;
	float relativeY      = properties.positionY / windowHeight;

	MeshBufferProperties bufferProperties{
		-1.0f + 2.0f*relativeX,						// left edge
		-1.0f + 2.0f*(relativeX + relativeWidth),	// right edge
		 1.0f - 2.0f*relativeY,			     		// top edge
		 1.0f - 2.0f*(relativeY + relativeHeight),  // bottom edge
	};
	CreateMeshBuffer(bufferProperties);
}

GLQuad::~GLQuad()
{
	glDeleteBuffers(1, &positionBuffer);
	glDeleteBuffers(1, &texCoordBuffer);
}

void GLQuad::Draw()
{
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void GLQuad::CreateMeshBuffer(MeshBufferProperties properties)
{
	float left = properties.left;
	float right = properties.right;
	float top = properties.top;
	float bottom = properties.bottom;

	const GLuint valuesPerPosition = 3;
	std::vector<float> positions = {
		// Triangle 1
		left, top, 0.0f,
		left, bottom, 0.0f,
		right, bottom, 0.0f,

		// Triangle 2
		right, bottom, 0.0f,
		right, top, 0.0f,
		left, top, 0.0f,
	};

	// UVs work top to bottom, V is reversed to get image in correct orientation
	const GLuint valuesPerCoord = 4;
	std::vector<float> tcoords = {
		0.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f,

		1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f
	};

	glBindVertexArray(vao);

	// Generate buffers
	glGenBuffers(1, &positionBuffer);
	glGenBuffers(1, &texCoordBuffer);

	// Load positions
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glEnableVertexAttribArray(positionAttribId);
	glVertexAttribPointer(positionAttribId, valuesPerPosition, GL_FLOAT, false, 0, 0);
	glBufferVector(GL_ARRAY_BUFFER, positions, GL_STATIC_DRAW);

	// Load UVs
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
	glEnableVertexAttribArray(texCoordAttribId);
	glVertexAttribPointer(texCoordAttribId, valuesPerCoord, GL_FLOAT, false, 0, 0);
	glBufferVector(GL_ARRAY_BUFFER, tcoords, GL_STATIC_DRAW);
}
