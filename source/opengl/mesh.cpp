#include "mesh.h"
#include "../core/application.h"

#pragma warning(push,0)
#include "../thirdparty/tiny_obj_loader.h"
#pragma warning(pop)

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/euler_angles.hpp"

#include <string>
#include <iostream>

const GLuint positionAttribId = 0;
const GLuint normalAttribId = 1;
const GLuint colorAttribId = 2;
const GLuint texCoordAttribId = 3;

glm::mat4 MeshTransform::ModelMatrix() const
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


GLLineStrips::GLLineStrips()
{
	glBindVertexArray(vao);

	// Generate buffers
	glGenBuffers(1, &positionBuffer);
	glGenBuffers(1, &indexBuffer);
	
	// Load positions
	int valuesPerPosition = 3; // glm::fvec3 has 3 floats
	glEnableVertexAttribArray(positionAttribId);
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glVertexAttribPointer(positionAttribId, valuesPerPosition, GL_FLOAT, false, 0, 0);

	// Index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

	SendToGPU();
}

GLLineStrips::~GLLineStrips()
{
	glDeleteBuffers(1, &positionBuffer);
	glDeleteBuffers(1, &indexBuffer);
}

void GLLineStrips::AddLineStrip(const std::vector<glm::fvec3>& points)
{
	if (points.size() == 0)
	{
		return; // because there is no data to add
	}

	size_t newLineStart = lineStrips.size();
	size_t newIndicesStart = indices.size();

	numStrips++;
	lineStrips.resize(lineStrips.size() + points.size());
	indices.resize(indices.size() + points.size() + 1); // include restart_index at the end
	for (size_t i = 0; i < points.size(); ++i)
	{
		lineStrips[newLineStart + i] = points[i];
		indices[newIndicesStart + i] = static_cast<unsigned int>(newLineStart + i);
	}

	indices[indices.size()-1] = RESTART_INDEX;
}

void GLLineStrips::Clear()
{
	lineStrips.clear();
	indices.clear();

	lineStrips.shrink_to_fit();
	indices.shrink_to_fit();

	SendToGPU();
}

void GLLineStrips::SendToGPU()
{
	glBindVertexArray(vao);

	// Positions
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glBufferVector(GL_ARRAY_BUFFER, lineStrips, GL_STATIC_DRAW);

	// Indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferVector(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW);
}

void GLLineStrips::Draw()
{
	if (lineStrips.size() == 0 || indices.size() == 0)
	{
		return; // because there is no data to render
	}

	/*
		See these references for primitive restart
			https://www.khronos.org/opengl/wiki/Vertex_Rendering#Common
			https://gist.github.com/roxlu/51fc685b0303ee55c05b3ad96992f3ec
	*/
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(RESTART_INDEX);

	{
		glBindVertexArray(vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
		glDrawElements(GL_LINE_STRIP, GLsizei(indices.size()), GL_UNSIGNED_INT, (GLvoid*)0);
	}

	glDisable(GL_PRIMITIVE_RESTART);
}

GLBezierStrips::GLBezierStrips()
{
	const GLuint bezierPositionAttribId = 0;
	const GLuint bezierNormalAttribId = 1;
	const GLuint bezierTangentAttribId = 2;
	const GLuint bezierTexcoordAttribId = 3;
	const GLuint bezierWidthAttribId = 4;

	glBindVertexArray(vao);

	// Generate buffers
	glGenBuffers(1, &positionBuffer);
	glGenBuffers(1, &normalBuffer);
	glGenBuffers(1, &tangentBuffer);
	glGenBuffers(1, &texcoordBuffer);
	glGenBuffers(1, &widthBuffer);

	glGenBuffers(1, &indexBuffer);

	// Load positions
	int valuesPerPosition = 3; // glm::fvec3 has 3 floats
	glEnableVertexAttribArray(bezierPositionAttribId);
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glVertexAttribPointer(bezierPositionAttribId, valuesPerPosition, GL_FLOAT, false, 0, 0);

	// Load normals
	valuesPerPosition = 3;
	glEnableVertexAttribArray(bezierNormalAttribId);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glVertexAttribPointer(bezierNormalAttribId, valuesPerPosition, GL_FLOAT, false, 0, 0);

	// Load tangents
	valuesPerPosition = 3;
	glEnableVertexAttribArray(bezierTangentAttribId);
	glBindBuffer(GL_ARRAY_BUFFER, tangentBuffer);
	glVertexAttribPointer(bezierTangentAttribId, valuesPerPosition, GL_FLOAT, false, 0, 0);

	// Load texcoords
	valuesPerPosition = 2; // glm::fvec2
	glEnableVertexAttribArray(bezierTexcoordAttribId);
	glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
	glVertexAttribPointer(bezierTexcoordAttribId, valuesPerPosition, GL_FLOAT, false, 0, 0);

	// Load widths
	valuesPerPosition = 1; // float
	glEnableVertexAttribArray(bezierWidthAttribId);
	glBindBuffer(GL_ARRAY_BUFFER, widthBuffer);
	glVertexAttribPointer(bezierWidthAttribId, valuesPerPosition, GL_FLOAT, false, 0, 0);

	// Index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

	SendToGPU();
}

GLBezierStrips::~GLBezierStrips()
{
	glDeleteBuffers(1, &positionBuffer);
	glDeleteBuffers(1, &normalBuffer);
	glDeleteBuffers(1, &tangentBuffer);
	glDeleteBuffers(1, &texcoordBuffer);
	glDeleteBuffers(1, &widthBuffer);

	glDeleteBuffers(1, &indexBuffer);
}

void GLBezierStrips::AddBezierStrip(
	const std::vector<glm::fvec3>& points,
	const std::vector<glm::fvec3>& normals,
	const std::vector<glm::fvec3>& tangents,
	const std::vector<glm::fvec2>& texcoords,
	const std::vector<float>& widths
)
{
	if (points.size() == 0)
	{
		return; // because there is no data to add
	}

	if (normals.size() != points.size() || tangents.size() != points.size() || texcoords.size() != points.size() || widths.size() != points.size())
	{
		return; // because of size mismatch
	}

	size_t newLineStart = controlPoints.size();
	size_t newIndicesStart = indices.size();

	numStrips++;

	size_t newVectorSize = controlPoints.size() + points.size();
	controlPoints.resize(newVectorSize);
	controlNormals.resize(newVectorSize);
	controlTangents.resize(newVectorSize);
	controlTexcoords.resize(newVectorSize);
	controlWidths.resize(newVectorSize);

	indices.resize(indices.size() + points.size() + 1); // include restart_index at the end
	for (size_t i = 0; i < points.size(); ++i)
	{
		controlPoints[newLineStart + i] = points[i];
		controlNormals[newLineStart + i] = normals[i];
		controlTangents[newLineStart + i] = tangents[i];
		controlTexcoords[newLineStart + i] = texcoords[i];
		controlWidths[newLineStart + i] = widths[i];

		indices[newIndicesStart + i] = static_cast<unsigned int>(newLineStart + i);
	}

	indices[indices.size() - 1] = RESTART_INDEX;
}

void GLBezierStrips::Clear()
{
	controlPoints.clear();
	controlNormals.clear();
	controlTangents.clear();
	controlTexcoords.clear();
	controlWidths.clear();

	indices.clear();

	controlPoints.shrink_to_fit();
	controlNormals.shrink_to_fit();
	controlTangents.shrink_to_fit();
	controlTexcoords.shrink_to_fit();
	controlWidths.shrink_to_fit();

	indices.shrink_to_fit();

	SendToGPU();
}

void GLBezierStrips::SendToGPU()
{
	glBindVertexArray(vao);

	// Positions
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glBufferVector(GL_ARRAY_BUFFER, controlPoints, GL_STATIC_DRAW);

	// Normals
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferVector(GL_ARRAY_BUFFER, controlNormals, GL_STATIC_DRAW);

	// Tangents
	glBindBuffer(GL_ARRAY_BUFFER, tangentBuffer);
	glBufferVector(GL_ARRAY_BUFFER, controlTangents, GL_STATIC_DRAW);

	// Texcoords
	glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
	glBufferVector(GL_ARRAY_BUFFER, controlTexcoords, GL_STATIC_DRAW);

	// Widths
	glBindBuffer(GL_ARRAY_BUFFER, widthBuffer);
	glBufferVector(GL_ARRAY_BUFFER, controlWidths, GL_STATIC_DRAW);

	// Indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferVector(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW);
}

void GLBezierStrips::Draw()
{
	if (controlPoints.size() == 0 || indices.size() == 0)
	{
		return; // because there is no data to render
	}

	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(RESTART_INDEX);

	{
		glBindVertexArray(vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
		glDrawElements(GL_LINE_STRIP, GLsizei(indices.size()), GL_UNSIGNED_INT, (GLvoid*)0);
	}

	glDisable(GL_PRIMITIVE_RESTART);
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

namespace GLMesh
{
	bool LoadOBJ(std::filesystem::path FilePath, GLTriangleMesh& OutMesh)
	{
		OutMesh.Clear();

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;

		std::string warn;
		std::string err;

		std::string inputfile{ FilePath.string() };
		bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, inputfile.c_str());

		if (!warn.empty())
		{
			// Ignore, we don't care about material warnings at the moment
			//std::cout << warn << std::endl;
		}

		if (!loaded || shapes.size() == 0)
		{
			if (!err.empty())
			{
				std::cerr << err << std::endl;
			}
			return false;
		}

		// Copy indices
		auto& MainMesh = shapes[0].mesh;
		OutMesh.indices.resize(MainMesh.indices.size());
		for (size_t i = 0; i < MainMesh.indices.size(); i++)
		{
			OutMesh.indices[i] = MainMesh.indices[i].vertex_index;
		}

		// Resize OutMesh data to match the obj contents
		size_t vertexcount = attrib.vertices.size() / 3;
		OutMesh.positions.resize(vertexcount);
		OutMesh.normals.resize(vertexcount);
		OutMesh.colors.resize(vertexcount);
		OutMesh.texCoords.resize(vertexcount);

		// Copy all data (could probably use memcpy, but meh)
		for (size_t i = 0; i < vertexcount; i++)
		{
			size_t last_vertex_index = (3 * i + 2);
			if (last_vertex_index < attrib.vertices.size())
			{
				OutMesh.positions[i] = glm::fvec3{ 
					attrib.vertices[3*i+0], 
					attrib.vertices[3*i+1], 
					attrib.vertices[3*i+2]
				};
			}

			size_t last_normal_index = (3 * i + 2);
			if (last_normal_index < attrib.normals.size())
			{
				OutMesh.normals[i] = glm::fvec3{
					attrib.normals[3 * i + 0],
					attrib.normals[3 * i + 1],
					attrib.normals[3 * i + 2]
				};
			}

			size_t last_color_index = (3 * i + 2);
			if (last_color_index < attrib.colors.size())
			{
				OutMesh.colors[i] = glm::fvec4{
					attrib.colors[3 * i + 0],
					attrib.colors[3 * i + 1],
					attrib.colors[3 * i + 2],
					1.0f
				};
			}

			size_t last_texcoord_index = (2 * i + 2);
			if (last_texcoord_index < attrib.texcoords.size())
			{
				OutMesh.texCoords[i] = glm::fvec4{
					attrib.texcoords[2 * i + 0],
					attrib.texcoords[2 * i + 1],
					0.0f,
					0.0f
				};
			}
		}

		OutMesh.SendToGPU();

		return true;
	}

	void AppendCoordinateAxis(GLLine& OutLines, const glm::fvec3& origin, glm::fvec3& x, const glm::fvec3& y, const glm::fvec3& z, float scale)
	{
		OutLines.AddLine(origin, origin + x*scale, glm::fvec4(1.0f, 0.0f, 0.0f, 1.0f));
		OutLines.AddLine(origin, origin + y*scale, glm::fvec4(0.0f, 1.0f, 0.0f, 1.0f));
		OutLines.AddLine(origin, origin + z*scale, glm::fvec4(0.0f, 0.0f, 1.0f, 1.0f));
	}

	void AppendCoordinateAxis(GLLine& OutLines, const glm::mat4& Transform, float scale)
	{
		glm::fvec3 origin{ Transform[3][0], Transform[3][1], Transform[3][2] };
		glm::fvec3 x{Transform[0]};
		glm::fvec3 y{Transform[1]};
		glm::fvec3 z{Transform[2]};
		AppendCoordinateAxis(OutLines, origin, x*scale, y*scale, z*scale);
	}
}
