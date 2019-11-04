#pragma once
#include "math.h"

enum class TurntableInputState
{
	Rotate,
	Zoom,
	Translate,
	COUNT
};

class TurntableController
{
protected:
	// Not owned by class
	class Camera* camera = nullptr;
	float yaw = 0.0f;
	float pitch = 0.0f;
	float distance = 1.0f;

public:
	glm::vec3 position = glm::vec3{ 0.0f };
	float sensitivity = 1.0f;
	bool clampPitch = false;

	TurntableInputState inputState = TurntableInputState::Rotate;

	TurntableController(class Camera& controlledCamera);
	~TurntableController() = default;

	void Set(float newYaw, float newPitch, float newDistance);
	void Offset(float yawOffset, float pitchOffset, float distanceOffset);
	void ApplyMouseInput(int deltaX, int deltaY);
	void SnapToOrigin();

protected:
	void UpdateCamera();
};