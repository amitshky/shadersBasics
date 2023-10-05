#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
	/**
	 * @param aspectRatio width / height
	 * @param position world position of the camera (default = (0.0, 0.0, 3.0))
	 * @param yFov vertical Field Of View (FOV) in degrees (default = 45)
	 * @param zNear near clip plane in z axis (default = 0.01 units)
	 * @param zFar far clip plane in z axis (default = 100.0 units)
	 */
	explicit Camera(float aspectRatio,
		glm::vec3 position = glm::vec3(0.0f, 0.0f, 1.0f),
		float yFov = 45.0f,
		float zNear = 0.01f,
		float zFar = 100.0f);

	void OnUpdate(float deltatime);

	[[nodiscard]] inline glm::vec3 GetPosition() const { return m_Position; }
	[[nodiscard]] inline glm::mat4 GetViewMatrix() const { return m_ViewMatrix; }
	[[nodiscard]] inline glm::mat4 GetInverseViewMatrix() const { return m_InverseViewMatrix; }
	[[nodiscard]] inline glm::mat4 GetProjectionMatrix() const { return m_ProjectionMatrix; }
	[[nodiscard]] inline glm::mat4 GetInverseProjectionMatrix() const { return m_InverseProjectionMatrix; }
	[[nodiscard]] inline glm::mat4 GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

	void SetAspectRatio(float aspectRatio) { m_AspectRatio = aspectRatio; }
	void SetPosition(glm::vec3 position) { m_Position = position; }

private:
	float m_AspectRatio;
	glm::vec3 m_Position;
	float m_FOVy;
	float m_Near;
	float m_Far;

	glm::vec3 m_ForwardDirection;
	glm::vec3 m_RightDirection;
	glm::vec3 m_UpDirection;

	glm::mat4 m_ViewMatrix{};
	glm::mat4 m_ProjectionMatrix{};
	glm::mat4 m_ViewProjectionMatrix{};

	glm::mat4 m_InverseViewMatrix{};
	glm::mat4 m_InverseProjectionMatrix{};

	glm::vec2 m_LastMousePosition{ 0.0f, 0.0f };

	// for resetting the camera
	const glm::vec3 m_BackupPosition;
	const float m_BackupFOVy;
	const float m_BackupNear;
	const float m_BackupFar;
	const glm::vec3 m_BackupForwardDirection;
	const glm::vec3 m_BackupRightDirection;
};