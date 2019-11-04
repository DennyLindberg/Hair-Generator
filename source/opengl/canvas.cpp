#include "canvas.h"
#include "program.h"
#include "../core/utilities.h"
#include "../core/application.h"

std::shared_ptr<GLProgram> canvasShader;

void DrawBresenhamLine(GLTexture& texture, float x1, float y1, float x2, float y2, Color& color)
{
	// Taken from Rosetta Code
	// https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C.2B.2B

	// Bresenham's line algorithm
	const bool steep = (fabs(y2 - y1) > fabs(x2 - x1));
	if (steep)
	{
		std::swap(x1, y1);
		std::swap(x2, y2);
	}

	if (x1 > x2)
	{
		std::swap(x1, x2);
		std::swap(y1, y2);
	}

	const float dx = x2 - x1;
	const float dy = fabs(y2 - y1);

	float error = dx / 2.0f;
	const int ystep = (y1 < y2) ? 1 : -1;
	int y = (int)y1;

	const int maxX = (int)x2;

	for (int x = (int)x1; x < maxX; x++)
	{
		if (steep)
		{
			texture.SetPixelSafe(y, x, color);
		}
		else
		{
			texture.SetPixelSafe(x, y, color);
		}

		error -= dy;
		if (error < 0)
		{
			y += ystep;
			error += dx;
		}
	}
}

Canvas2D::Canvas2D()
{
	GLQuadProperties properties;
	properties.MatchWindowDimensions();
	Initialize(properties);
}

Canvas2D::Canvas2D(int width, int height)
{
	Initialize(GLQuadProperties{ 0.0f, 0.0f, float(width), float(height) });
}

Canvas2D::Canvas2D(GLQuadProperties properties)
{
	Initialize(properties);
}

void Canvas2D::RenderToScreen()
{
	if (bDirty)
	{
		bDirty = false;
		texture->CopyToGPU();
	}

	canvasShader->Use();
	texture->UseForDrawing();
	quad->Draw();
}

void Canvas2D::Initialize(GLQuadProperties properties)
{
	minX = int(properties.positionX);
	minY = int(properties.positionY);
	maxX = minX + int(properties.width);
	maxY = minY + int(properties.height);

	if (!canvasShader)
	{
		ApplicationSettings s = GetApplicationSettings();

		canvasShader = std::make_shared<GLProgram>();
		std::string fragment, vertex;
		if (LoadText(s.contentPath/"basic_fragment.glsl", fragment) && LoadText(s.contentPath/"basic_vertex.glsl", vertex))
		{
			canvasShader->LoadFragmentShader(fragment);
			canvasShader->LoadVertexShader(vertex);
			canvasShader->CompileAndLink();
		}
	}

	quad = std::make_shared<GLQuad>(properties);
	texture = std::make_shared<GLTexture>(int(properties.width), int(properties.height));
}

/*
	Canvas drawing methods
*/
void Canvas2D::Fill(Color& color)
{
	bDirty = true;
	texture->Fill(color);
}

void Canvas2D::DrawLine(glm::fvec2 start, glm::fvec2 end, Color& color)
{
	bDirty = true;
	DrawBresenhamLine(*texture, start.x, start.y, end.x, end.y, color);
}