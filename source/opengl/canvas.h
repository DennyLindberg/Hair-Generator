#pragma once
#include <algorithm>
#include <memory>
#include "texture.h"
#include "mesh.h"
#include "../core/math.h"

class Canvas2D
{
protected:
	bool bDirty = true;
	std::shared_ptr<GLQuad> quad;
	std::shared_ptr<GLTexture> texture;

	int minX;
	int maxX;
	int minY;
	int maxY;

public:
	Canvas2D();
	Canvas2D(int width, int height);
	Canvas2D(GLQuadProperties properties);
	~Canvas2D() = default;

	std::shared_ptr<GLTexture> GetTexture()
	{
		return texture;
	}

	void RenderToScreen();

protected:
	void Initialize(GLQuadProperties properties);

/*
	Canvas drawing methods
*/
public:
	void Fill(Color& color);
	void DrawLine(glm::fvec2 start, glm::fvec2 end, Color& color);
};