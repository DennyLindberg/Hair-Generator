#pragma once
#include <vector>
#include "glad/glad.h"
#include "../core/math.h"
#include <filesystem>

struct GLQuadProperties
{
	float positionX;
	float positionY;
	float width;
	float height;

	void MatchWindowDimensions();
};

struct MeshTransform
{
	glm::fvec3 position{ 0.0f };
	glm::fvec3 rotation{ 0.0f };
	glm::fvec3 scale{ 1.0f };

	glm::mat4 ModelMatrix() const;
};

class GLMeshInterface
{
protected:
	GLuint vao = 0;

public:
	MeshTransform transform;

	GLMeshInterface();
	~GLMeshInterface();

	// Behaves like glBufferData, but for std::vector<T>.
	template <class T>
	void glBufferVector(GLenum glBufferType, const std::vector<T>& vector, GLenum usage = GL_STATIC_DRAW)
	{
		size_t count = vector.size();
		float* frontPtr = (float*)((count > 0) ? &vector.front() : NULL);
		glBufferData(glBufferType, count*sizeof(T), frontPtr, usage);
	}
};

class GLTriangleMesh : public GLMeshInterface
{
protected:
	bool allocated = false;
	GLuint positionBuffer = 0;
	GLuint normalBuffer = 0;
	GLuint colorBuffer = 0;
	GLuint texCoordBuffer = 0;
	GLuint indexBuffer = 0;

public:
	std::vector<glm::fvec3> positions;
	std::vector<glm::fvec3> normals;
	std::vector<glm::fvec4> colors;
	std::vector<glm::fvec4> texCoords;
	std::vector<unsigned int> indices;

	GLTriangleMesh(bool allocate = true);
	~GLTriangleMesh();

	void Clear();
	void SendToGPU();
	void Draw();
	void AddVertex(glm::fvec3 pos, glm::fvec4 color, glm::fvec4 texcoord);
	void AddVertex(glm::fvec3 pos, glm::fvec3 normal, glm::fvec4 color, glm::fvec4 texcoord);
	void DefineNewTriangle(unsigned int index1, unsigned int index2, unsigned int index3);
	void AppendMesh(const GLTriangleMesh& other);
	void AppendMeshTransformed(const GLTriangleMesh& other, glm::mat4 transform);
	void ApplyMatrix(glm::mat4 transform, int firstIndex, int lastIndex);
	void ApplyMatrix(glm::mat4 transform);
};

struct GLLineSegment
{
	glm::fvec3 start;
	glm::fvec3 end;
};

class GLLine : public GLMeshInterface
{
protected:
	GLuint positionBuffer = 0;
	GLuint colorBuffer = 0;

	std::vector<GLLineSegment> lineSegments;
	std::vector<glm::fvec4> colors;

public:
	GLLine();

	~GLLine();

	void AddLine(glm::fvec3 start, glm::fvec3 end, glm::fvec4 color);

	void Clear();

	void SendToGPU();

	void Draw();
};

class GLLineStrips : public GLMeshInterface
{
protected:
	const GLuint RESTART_INDEX = 0xFFFF;

	GLuint positionBuffer = 0;
	GLuint indexBuffer = 0;

	unsigned int numStrips = 0;
	std::vector<glm::fvec3> lineStrips; // each line strip is separated by the RESTART_INDEX in indices
	std::vector<unsigned int> indices;

public:
	GLLineStrips();
	~GLLineStrips();

	void AddLineStrip(const std::vector<glm::fvec3>& points);

	void Clear();

	void SendToGPU();

	void Draw();
};

class GLBezierStrips : public GLMeshInterface
{
protected:
	const GLuint RESTART_INDEX = 0xFFFF;

	GLuint positionBuffer = 0;
	GLuint normalBuffer = 0;
	GLuint tangentBuffer = 0;
	GLuint texcoordBuffer = 0;
	GLuint widthBuffer = 0;
	GLuint thicknessBuffer = 0;
	GLuint shapeBuffer = 0;
	GLuint subdivisionsBuffer = 0;

	GLuint indexBuffer = 0;

	unsigned int numStrips = 0;
	std::vector<glm::fvec3> controlPoints; // each curve is separated on the GPU by the RESTART_INDEX in indices
	std::vector<glm::fvec3> controlNormals;
	std::vector<glm::fvec3> controlTangents;
	std::vector<glm::fvec3> controlTexcoords; // {ustart, v, uend}
	std::vector<float> controlWidths;
	std::vector<float> controlThickness;
	std::vector<int>   controlShapes;
	std::vector<int>   controlSubdivisions;

	std::vector<unsigned int> indices;

public:
	GLBezierStrips();
	~GLBezierStrips();

	bool AddBezierStrip(
		const std::vector<glm::fvec3>& points,
		const std::vector<glm::fvec3>& normals,
		const std::vector<glm::fvec3>& tangents,
		const std::vector<glm::fvec3>& texcoords,
		const std::vector<float>& widths,
		const std::vector<float>& thickness,
		const std::vector<int>& shapes,
		const std::vector<int>& subdivisions
	);

	void Clear();

	void SendToGPU();

	void Draw();
};

class GLQuad : public GLMeshInterface
{
protected:
	GLuint positionBuffer = 0;
	GLuint texCoordBuffer = 0;

public:
	GLQuad();
	GLQuad(GLQuadProperties properties);
	~GLQuad();
	void Draw();

protected:
	struct MeshBufferProperties
	{
		float left;
		float right;
		float top;
		float bottom;
	};

	void CreateMeshBuffer(MeshBufferProperties properties);
};

namespace GLMesh
{
	bool LoadOBJ(std::filesystem::path FilePath, class GLTriangleMesh& OutMesh);
	bool LoadCurves(std::filesystem::path FilePath, class GLBezierStrips& OutStrips);
	void AppendCoordinateAxis(GLLine& OutLines, const glm::fvec3& origin, glm::fvec3& x, const glm::fvec3& y, const glm::fvec3& z, float scale = 1.0f);
	void AppendCoordinateAxis(GLLine& OutLines, const glm::mat4& Transform, float scale = 1.0f);
}
