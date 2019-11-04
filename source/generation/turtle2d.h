#pragma once
#include "../opengl/canvas.h"
#include "../core/math.h"
#include <map>
#include <stack>
#include <functional>

template<class OptionalState = int>
struct TurtleState2D
{
	glm::fvec2 position;
	float angle;
	OptionalState properties;
};

template<class OptionalState = int>
class Turtle2D
{
protected:
	using TurtleState = TurtleState2D<OptionalState>;

public:
	TurtleState state;

	std::stack<TurtleState> turtleStack;
	std::map<char, std::function<void(Turtle2D&, Canvas2D&)>> actions;

	Turtle2D() = default;
	~Turtle2D() = default;

	void Clear()
	{
		turtleStack = std::stack<TurtleState>();
	}

	void Draw(Canvas2D& canvas, std::string& symbols, glm::fvec2 startPosition, float startAngle)
	{
		if (turtleStack.size() != 0)
		{
			Clear();
		}
		state.position = startPosition;
		state.angle = startAngle;

		for (char& c : symbols)
		{
			if (actions.count(c))
			{
				actions[c](*this, canvas);
			}
		}
	}

	void PushState()
	{
		turtleStack.push(state);
	}

	void PopState()
	{
		state = turtleStack.top();
		turtleStack.pop();
	}

	glm::fvec2 GetDirection()
	{
		// Angle is flipped so that the system is right handed
		float radians = -state.angle * PI_f / 180.0f;
		return glm::fvec2{
			cosf(radians),
			sinf(radians)
		};
	}

	void Rotate(float degrees)
	{
		state.angle += degrees;
	}
};