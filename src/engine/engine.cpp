#include "engine/engine.h"
#include "core/core.h"
#include "core/input.h"
#include "engine/types.h"
#include "engine/initializers.h"
#include "utils/utils.h"


Engine* Engine::s_Instance = nullptr;

Engine::Engine(const char* title, const uint64_t width, const uint64_t height)
{
	s_Instance = this;
	Init(title, width, height);
}

Engine::~Engine()
{
	Cleanup();
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
	Logger::Info("{} application initialized!", title);

	m_Window = std::make_unique<Window>(WindowProps{ title, width, height });
	// set window event callbacks
	m_Window->SetCloseEventCallbackFn(BIND_EVENT_FN(Engine::OnCloseEvent));
	m_Window->SetResizeEventCallbackFn(BIND_EVENT_FN(Engine::OnResizeEvent));
	m_Window->SetMouseEventCallbackFn(BIND_EVENT_FN(Engine::OnMouseMoveEvent));
	m_Window->SetKeyEventCallbackFn(BIND_EVENT_FN(Engine::OnKeyEvent));

	CreateVulkanInstance(title);
	SetupDebugMessenger();
	m_Window->CreateWindowSurface(m_VulkanInstance);
}

void Engine::Cleanup()
{
	m_Window->DestroyWindowSurface(m_VulkanInstance);
	if (VulkanConfig::enableValidationLayers)
		DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);
	vkDestroyInstance(m_VulkanInstance, nullptr);
}

void Engine::CreateVulkanInstance(const char* title)
{
	THROW(VulkanConfig::enableValidationLayers && !utils::CheckValidationLayerSupport(),
		"Validation layers requested, but not available!")

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = title;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0; // the highest version the application is designed to use

	VkInstanceCreateInfo instanceInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	// get required extensions
	std::vector<const char*> extensions = utils::GetRequiredExtensions();
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceInfo.ppEnabledExtensionNames = extensions.data();

	// specify global validation layers
	if (VulkanConfig::enableValidationLayers)
	{
		instanceInfo.enabledLayerCount = static_cast<uint32_t>(VulkanConfig::validationLayers.size());
		instanceInfo.ppEnabledLayerNames = VulkanConfig::validationLayers.data();

		// debug messenger
		VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
		initializers::DebugMessengerCreateInfo(debugMessengerInfo);
		instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerInfo;
	}
	else
	{
		instanceInfo.enabledLayerCount = 0;
		instanceInfo.pNext = nullptr;
	}

	THROW(
		vkCreateInstance(&instanceInfo, nullptr, &m_VulkanInstance) != VK_SUCCESS, "Failed to create Vulkan instance!")
}

void Engine::SetupDebugMessenger()
{
	if (!VulkanConfig::enableValidationLayers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
	initializers::DebugMessengerCreateInfo(debugMessengerInfo);

	THROW(CreateDebugUtilsMessengerEXT(m_VulkanInstance, &debugMessengerInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS,
		"Failed to setup debug messenger!");
}

VkResult Engine::CreateDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	// requires a valid instance to have been created
	// so we cannot debug any issues in vkCreateInstance

	// get the function pointer for creating debug utils messenger
	// returns nullptr if the function couldn't be loaded
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void Engine::DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	// must be destroyed before instance is destroyed
	// so we cannot debug any issues in vkDestroyInstance
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, debugMessenger, pAllocator);
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