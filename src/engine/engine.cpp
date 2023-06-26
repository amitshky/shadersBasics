#include "engine/engine.h"
#include "core/core.h"
#include "core/input.h"
#include "engine/types.h"


Engine* Engine::s_Instance = nullptr;

Engine::Engine(const char* title, const uint64_t width, const uint64_t height)
{
	s_Instance = this;
	Init(title, width, height);
}

Engine* Engine::Create(const char* title, const uint64_t width, const uint64_t height)
{
	if (s_Instance == nullptr)
		return new Engine(title, width, height);

	return s_Instance;
}

void Engine::Run()
{
	while (m_IsRunning)
	{
		ProcessInput();
		m_Window->OnUpdate();
	}
}

void Engine::Init(const char* title, const uint64_t width, const uint64_t height)
{
	s_Instance = this;
	Logger::Info("{} application initialized!", title);

	m_Window = std::make_unique<Window>(WindowProps{ title, width, height });
	// set window event callbacks
	m_Window->SetCloseEventCallbackFn(BIND_EVENT_FN(Engine::OnCloseEvent));
	m_Window->SetResizeEventCallbackFn(BIND_EVENT_FN(Engine::OnResizeEvent));
	m_Window->SetMouseEventCallbackFn(BIND_EVENT_FN(Engine::OnMouseMoveEvent));
	m_Window->SetKeyEventCallbackFn(BIND_EVENT_FN(Engine::OnKeyEvent));
}

void Engine::ProcessInput()
{
	// forward input data to ImGui first
	// ImGuiIO& io = ImGui::GetIO();
	// if (io.WantCaptureMouse || io.WantCaptureKeyboard)
	// 	return;

	if (Input::IsMouseButtonPressed(Mouse::BUTTON_1))
	{
		// hide cursor when moving camera
		m_Window->HideCursor();
	}
	else if (Input::IsMouseButtonReleased(Mouse::BUTTON_1))
	{
		// unhide cursor when camera stops moving
		m_Window->ShowCursor();
	}
}

void Engine::OnCloseEvent()
{
	m_IsRunning = false;
}

void Engine::OnResizeEvent(int width, int height)
{}

void Engine::OnMouseMoveEvent(double xpos, double ypos)
{
	// ImGuiIO& io = ImGui::GetIO();
	// if (io.WantCaptureMouse)
	// 	return;
}

void Engine::OnKeyEvent(int key, int scancode, int action, int mods)
{
	// quits the application
	// works even when the ui is in focus
	if (Input::IsKeyPressed(Key::LEFT_CONTROL) && Input::IsKeyPressed(Key::Q))
		m_IsRunning = false;

	// 	ImGuiIO& io = ImGui::GetIO();
	// 	if (io.WantCaptureKeyboard)
	// 		return;
}