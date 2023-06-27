#pragma once

#include <cstdint>
#include <memory>
#include <vulkan/vulkan.h>
#include "core/window.h"

class Engine
{
public:
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;
	~Engine();

	[[nodiscard]] static Engine* Create(const char* title, const uint64_t width = 1280, const uint64_t height = 720);
	[[nodiscard]] static inline Engine* GetInstance() { return s_Instance; }
	[[nodiscard]] static inline GLFWwindow* GetWindowHandle() { return s_Instance->m_Window->GetWindowHandle(); }

	void Run();

private:
	explicit Engine(const char* title, const uint64_t width = 1280, const uint64_t height = 720);

	void Init(const char* title, const uint64_t width, const uint64_t height);
	void Cleanup();

	void CreateVulkanInstance(const char* title);
	void SetupDebugMessenger();

	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	void ProcessInput();
	void OnCloseEvent();
	void OnResizeEvent(int width, int height);
	void OnMouseMoveEvent(double xpos, double ypos);
	void OnKeyEvent(int key, int scancode, int action, int mods);

private:
	bool m_IsRunning = true;
	static Engine* s_Instance;
	std::unique_ptr<Window> m_Window{};

	VkInstance m_VulkanInstance;
	VkDebugUtilsMessengerEXT m_DebugMessenger;
};