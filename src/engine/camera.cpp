#include "engine/camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "imgui/imgui.h"
#include "core/input.h"

Camera::Camera(float aspectRatio, glm::vec3 position, float yFov, float zNear, float zFar)
	: m_AspectRatio{ aspectRatio },
	  m_Position{ position },
	  m_FOVy{ yFov },
	  m_Near{ zNear },
	  m_Far{ zFar },
	  m_ForwardDirection{ 0.0f, 0.0f, -1.0f },
	  m_RightDirection{ 1.0f, 0.0f, 0.0f },
	  m_UpDirection{ 0.0f, 1.0f, 0.0f }
{}

void Camera::OnUpdate(float deltatime)
{
	glm::vec2 mousePos = Input::GetMousePosition();
	glm::vec2 deltaMousePos = (mousePos - m_LastMousePosition) * 0.01f;
	m_LastMousePosition = mousePos;

	m_RightDirection = glm::cross(m_ForwardDirection, m_UpDirection);

	// calc matrices
	m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_ForwardDirection, m_UpDirection);
	m_ProjectionMatrix = glm::perspective(m_FOVy, m_AspectRatio, m_Near, m_Far);
	m_ProjectionMatrix[1][1] *= -1; // flip y-coord
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

	m_InverseViewMatrix = glm::inverse(m_ViewMatrix);
	m_InverseProjectionMatrix = glm::inverse(m_ProjectionMatrix);

	// if ImGui is in focus, don't take keyboard input for camera
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard)
		return;

	if (!Input::IsMouseButtonPressed(Mouse::BUTTON_1))
	{
		Input::SetCursorMode(CursorMode::NORMAL);
		return;
	}

	Input::SetCursorMode(CursorMode::DISABLED);

	// movement
	const float speed = 3.0f * (deltatime / 1000.0f);
	if (Input::IsKeyPressed(Key::W)) // forward
		m_Position += m_ForwardDirection * speed;
	else if (Input::IsKeyPressed(Key::S)) // backward
		m_Position -= m_ForwardDirection * speed;
	if (Input::IsKeyPressed(Key::A)) // left
		m_Position -= m_RightDirection * speed;
	else if (Input::IsKeyPressed(Key::D)) // right
		m_Position += m_RightDirection * speed;
	if (Input::IsKeyPressed(Key::E)) // up
		m_Position += m_UpDirection * speed;
	else if (Input::IsKeyPressed(Key::Q)) // down
		m_Position -= m_UpDirection * speed;

	// rotation
	if (deltaMousePos.x != 0.0f || deltaMousePos.y != 0.0f)
	{
		float pitchDelta = deltaMousePos.y * 0.3f;
		float yawDelta = deltaMousePos.x * 0.3f;

		glm::quat quaternion = glm::normalize(glm::cross(
			glm::angleAxis(-pitchDelta, m_RightDirection), glm::angleAxis(-yawDelta, glm::vec3(0.0f, 1.0f, 0.0f))));
		m_ForwardDirection = glm::rotate(quaternion, m_ForwardDirection);
	}
}