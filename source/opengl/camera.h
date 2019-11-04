#pragma once
#include "../core/math.h"
#include "../core/application.h"

// This camera uses a position and a focus point to determine orientation.
// The getters and setters are used to ensure that the internals update.
class Camera
{
protected:
	bool bDirty = true;
	glm::vec3 forwardVector = { 0.0f, 0.0f, 1.0f };
	glm::vec3 upVector = { 0.0f, 1.0f, 0.0f };
	glm::vec3 sideVector = { 1.0f, 0.0f, 0.0f };

	glm::vec3 position = glm::vec3{ 0.0f, 0.0f, 1.0f };
	glm::vec3 focusPoint = glm::vec3{ 0.0f };

public:
	bool flipUpDirection = false;
	float fieldOfView = 90.0f;
	float nearClipPlane = 0.1f;
	float farClipPlane = 100.0f;

	Camera() = default;
	~Camera() = default;

	glm::fvec3 GetPosition()
	{
		return position;
	}

	void SetPosition(glm::vec3& newPosition)
	{
		position = newPosition;
		UpdateVectors();
	}

	void SetFocusPoint(glm::vec3& newFocusPoint)
	{
		focusPoint = newFocusPoint;
		UpdateVectors();
	}

	inline glm::mat4 ViewMatrix()
	{
		return glm::lookAt(position, focusPoint, upVector);
	}

	inline glm::mat4 ProjectionMatrix()
	{
		return glm::perspective(
			glm::radians(fieldOfView),
			GetApplicationSettings().windowRatio,
			nearClipPlane, farClipPlane
		);
	}

	inline glm::mat4 ViewProjectionMatrix()
	{
		return ProjectionMatrix() * ViewMatrix();
	}

	inline glm::vec3 UpVector() { return upVector; }
	inline glm::vec3 SideVector() { return sideVector; }
	inline glm::vec3 ForwardVector() { return forwardVector; }

protected:
	void UpdateVectors()
	{
		forwardVector = glm::normalize(focusPoint - position);

		glm::vec3 worldUp = glm::vec3(0.0f, flipUpDirection? -1.0f : 1.0f, 0.0f);
		if (!glm::all(glm::equal(forwardVector, worldUp)))
		{
			sideVector = glm::cross(forwardVector, worldUp);
			upVector = glm::cross(sideVector, forwardVector);

			sideVector = glm::normalize(sideVector);
			upVector = glm::normalize(upVector);
		}
	}
};