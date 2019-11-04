#include "input.h"
#include "../opengl/camera.h"

TurntableController::TurntableController(Camera& controlledCamera)
{
	camera = &controlledCamera;
}

void TurntableController::Set(float newYaw, float newPitch, float newDistance)
{
	yaw = newYaw;
	pitch = newPitch;
	distance = newDistance;
	UpdateCamera();
}

void TurntableController::Offset(float yawOffset, float pitchOffset, float distanceOffset)
{
	yaw += camera->flipUpDirection? -yawOffset : yawOffset;
	pitch += pitchOffset;
	distance += distanceOffset;
	UpdateCamera();
}

void TurntableController::ApplyMouseInput(int deltaX, int deltaY)
{
	float relativeSensitivity = 0.1f*log(1.0f + distance);

	switch (inputState)
	{
	case TurntableInputState::Zoom:
	{
		float normalizedMovement = (deltaX - deltaY) / 2.0f;
		Offset(0.0f, 0.0f, normalizedMovement*sensitivity*relativeSensitivity);
		break;
	}
	case TurntableInputState::Translate:
	{
		glm::vec3 inputOffset = camera->UpVector()*float(deltaY) + camera->SideVector()*float(deltaX);
		inputOffset *= 0.5f*sensitivity*relativeSensitivity;
		position += inputOffset;
		UpdateCamera();
		break;
	}
	case TurntableInputState::Rotate:
	default:
	{
		Offset(deltaX*sensitivity, deltaY*sensitivity, 0.0f);
	}
	}
}

void TurntableController::SnapToOrigin()
{
	position = glm::vec3{ 0.0f };
	UpdateCamera();
}

glm::vec3 PlacementVector(float yaw, float pitch)
{
	// Angle is inverted to create a right handed system
	float yawRad = -yaw * PI_f / 180.0f;
	float pitchRad = pitch * PI_f / 180.0f;

	float b = cosf(pitchRad);
	return glm::vec3{
		b*cosf(yawRad),
		sinf(pitchRad),
		b*sinf(yawRad)
	};
}

void TurntableController::UpdateCamera()
{
	yaw = fmod(yaw, 360.0f);
	pitch = fmod(pitch, 360.0f);

	if (distance < camera->nearClipPlane)
	{
		distance = camera->nearClipPlane;
	}

	if (clampPitch)
	{
		if (pitch > 90.0f)  pitch = 90.0f;
		if (pitch < -90.0f) pitch = -90.0f;
	}
	else
	{
		camera->flipUpDirection = (abs(pitch) > 90.0f && abs(pitch) < 270.0f);
	}

	camera->SetPosition(position + PlacementVector(yaw, pitch) * distance);
	camera->SetFocusPoint(position);
}

