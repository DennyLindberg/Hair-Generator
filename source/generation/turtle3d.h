#pragma once
#include "../opengl/mesh.h"
#include "../core/math.h"
#include <map>
#include <stack>
#include <functional>

template<class OptionalState = int>
struct TurtleTransform
{
	glm::fvec3 position = glm::fvec3{ 0.0f };
	glm::fvec3 forward = glm::fvec3{ 0.0f, 1.0f, 0.0f }; // the direction the turtle is travelling (Y+ by default)
	glm::fvec3 up = glm::fvec3{ 0.0f, 0.0f, 1.0f };	     // orthogonal to the forward direction, used for orientation (Z+ by default) 
	OptionalState properties;

	void Clear()
	{
		position = glm::fvec3{ 0.0f };
		forward = glm::fvec3{ 0.0f, 1.0f, 0.0f };
		up = glm::fvec3{ 0.0f, 0.0f, 1.0f };
	}
};

template<class OptionalState = int>
struct Bone
{
	using TBone = Bone<OptionalState>;

	TBone* parent = nullptr;
	TBone* firstChild = nullptr;
	TBone* lastChild = nullptr;
	TBone* previousSibling = nullptr;
	TBone* nextSibling = nullptr;

	TurtleTransform<OptionalState> transform;
	float length = 0.0f;
	int nodeDepth = 1; // distance from root bone in the node tree

	Bone() = default;
	~Bone()
	{
		Clear();
	}

	glm::fvec3 tipPosition()
	{
		return transform.position + transform.forward*length;
	}

	void Clear()
	{
		parent = nullptr;
		transform.Clear();
		length = 0.0f;

		if (firstChild)
		{
			delete firstChild;
			firstChild = nullptr;
		}

		if (nextSibling)
		{
			delete nextSibling;
			nextSibling = nullptr;
		}
	}

	TBone* NewChild()
	{
		TBone* newChild = new TBone();
		if (!firstChild)
		{
			firstChild = newChild;
			lastChild = newChild;
		}
		else
		{
			newChild->previousSibling = lastChild;
			lastChild->nextSibling = newChild;
			lastChild = newChild;
		}

		newChild->parent = this;
		newChild->transform.position = this->tipPosition();
		newChild->nodeDepth = this->nodeDepth + 1;

		return newChild;
	}

	void DebugPrint(int depth = 0)
	{
		for (int i = 0; i < depth; ++i)
		{
			printf("  ");
		}

		printf("%d\n", nodeDepth);

		if (firstChild)
		{
			firstChild->DebugPrint(depth+1);
		}

		if (nextSibling)
		{
			nextSibling->DebugPrint(depth);
		}
	}

	void ForEach(std::function<void(TBone*)>& callback)
	{
		callback(this);

		if (firstChild)
		{
			firstChild->ForEach(callback);
		}

		if (nextSibling)
		{
			nextSibling->ForEach(callback);
		}
	}
};

template<class OptionalState = int>
class Turtle3D
{
protected:
	using TurtleBone = Bone<OptionalState>;
	using TTransform = TurtleTransform<OptionalState>;

public:
	std::map<char, std::function<void(Turtle3D&, int)>> actions;

	TTransform transform;
	std::stack<TTransform> transformStack;
	std::stack<TurtleBone*> branchStack;
	TurtleBone* activeBone = nullptr;
	TurtleBone* rootBone = nullptr;

	int boneCount = 0;

	Turtle3D()
	{
		Clear();
	}

	~Turtle3D() = default;

	void Clear()
	{
		transform.Clear();
		boneCount = 0;
		if (rootBone)
		{
			delete rootBone;
			rootBone = nullptr;
			activeBone = nullptr;
		}
		transformStack = std::stack<TTransform>();
		branchStack = std::stack<TurtleBone*>();
	}

	void GenerateSkeleton(std::string& symbols, TTransform startTransform = TTransform{})
	{
		Clear();
		transform = std::move(startTransform);

		size_t size = symbols.size();
		for (int i=0; i<size; i++)
		{
			int repetitionCounter = 1;
			while ((i < int(size - 1)) && symbols[i] == symbols[i + 1])
			{
				repetitionCounter++;
				i++;
			}

			if (actions.count(symbols[i]))
			{
				actions[symbols[i]](*this, repetitionCounter);
			}
		}
	}

	void PushState()
	{
		branchStack.push(activeBone);

		transformStack.push(transform);
	}

	void PopState()
	{
		transform = transformStack.top();
		transformStack.pop();

		activeBone = branchStack.top();
		branchStack.pop();
	}

	// Angles are related to the forward and up basis vectors. (Roll is applied first)
	void Rotate(float rollDegrees, float pitchDegrees)
	{
		glm::mat4 identity{ 1 };

		// Roll the forward direction
		auto rollRot = glm::rotate(identity, glm::radians(rollDegrees), transform.forward);
		transform.up = rollRot * glm::fvec4(transform.up, 0.0f);

		// Change the up/down angle of the forward direction and up direction
		glm::fvec3 pitchVector = glm::cross(transform.forward, transform.up);
		Rotate(pitchDegrees, pitchVector);
	}

	void Rotate(float degrees, glm::fvec3 rotateVector)
	{
		auto rotateMatrix = glm::rotate(glm::mat4{ 1.0f }, glm::radians(degrees), rotateVector);
		transform.forward = rotateMatrix * glm::fvec4(transform.forward, 0.0f);
		transform.up = rotateMatrix * glm::fvec4(transform.up, 0.0f);
	}

	void MoveForward(float distance)
	{
		PushBone(distance);
		transform.position += transform.forward*distance;
	}

	void PushBone(float length)
	{
		if (!rootBone)
		{
			rootBone = new TurtleBone{};
			activeBone = rootBone;
		}
		else
		{
			activeBone = activeBone->NewChild();
		}
		activeBone->transform = transform;
		activeBone->length = length;
		boneCount++;
	}

	void ForEachBone(std::function<void(TurtleBone*)> callback)
	{
		if (rootBone && callback)
		{
			rootBone->ForEach(callback);
		}
	}

	void BonesToGLLines(GLLine& lines, glm::fvec4 boneColor, glm::fvec4 normalColor)
	{
		ForEachBone([&lines, &boneColor, &normalColor](TurtleBone* b) -> void
		{
			lines.AddLine(
				b->transform.position,
				b->tipPosition(),
				boneColor
			);
			lines.AddLine(
				b->transform.position,
				b->transform.position + b->transform.up*0.2f,
				normalColor
			);
		});
		lines.SendToGPU();
	}
};