#include "core/input.h"

#include <GLFW/glfw3.h>
#include "engine/engine.h"


bool Input::IsKeyPressed(Key keycode)
{
	GLFWwindow* window = Engine::GetWindowHandle();
	auto status = glfwGetKey(window, static_cast<int>(keycode));
	return status == GLFW_PRESS || status == GLFW_REPEAT;
}

bool Input::IsKeyReleased(Key keycode)
{
	GLFWwindow* window = Engine::GetWindowHandle();
	auto status = glfwGetKey(window, static_cast<int>(keycode));
	return status == GLFW_RELEASE;
}

bool Input::IsMouseButtonPressed(Mouse button)
{
	GLFWwindow* window = Engine::GetWindowHandle();
	auto status = glfwGetMouseButton(window, static_cast<int>(button));
	return status == GLFW_PRESS;
}

bool Input::IsMouseButtonReleased(Mouse button)
{
	GLFWwindow* window = Engine::GetWindowHandle();
	auto status = glfwGetMouseButton(window, static_cast<int>(button));
	return status == GLFW_RELEASE;
}

void Input::SetCursorMode(CursorMode mode)
{
	GLFWwindow* window = Engine::GetWindowHandle();
	glfwSetInputMode(window, GLFW_CURSOR, static_cast<int>(mode));
}

glm::vec2 Input::GetMousePosition()
{
	GLFWwindow* window = Engine::GetWindowHandle();
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	return { static_cast<float>(xpos), static_cast<float>(ypos) };
}

float Input::GetMouseX()
{
	auto pos = Input::GetMousePosition();
	return pos.x;
}

float Input::GetMouseY()
{
	auto pos = Input::GetMousePosition();
	return pos.y;
}
