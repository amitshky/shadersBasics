#include "engine/camera.h"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(float aspectRatio, glm::vec3 position, float yFov, float zNear, float zFar)
	: m_AspectRatio{ aspectRatio },
	  m_Position{ position },
	  m_FOVy{ yFov },
	  m_Near{ zNear },
	  m_Far{ zFar },
	  m_ForwardDirection{ 0.0f, 0.0f, -1.0f },
	  m_RightDirection{ 1.0f, 0.0f, 0.0f },
	  m_UpDirection{ 0.0f, 0.0f, 1.0f }
{}

void Camera::OnUpdate(float deltatime)
{
	m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_ForwardDirection, glm::vec3(0.0f, 1.0f, 0.0f));
	m_ProjectionMatrix = glm::perspective(m_FOVy, m_AspectRatio, m_Near, m_Far);
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

	m_InverseViewMatrix = glm::inverse(m_ViewMatrix);
	m_InverseProjectionMatrix = glm::inverse(m_ProjectionMatrix);
}