#pragma once

#include <cstdint>
#include <memory>
#include <vulkan/vulkan.h>
#include "core/window.h"
#include "engine/types.h"

class Engine
{
public:
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;
	~Engine();

	[[nodiscard]] static Engine* Create(const char* title, const uint64_t width = 1280, const uint64_t height = 720);
	[[nodiscard]] static inline Engine* GetInstance() { return s_Instance; }
	[[nodiscard]] static inline GLFWwindow* GetWindowHandle() { return s_Instance->m_Window->GetWindowHandle(); }
	[[nodiscard]] static inline VkPhysicalDevice GetPhysicalDevice() { return s_Instance->m_PhysicalDevice; }

	void Run();

private:
	explicit Engine(const char* title, const uint64_t width = 1280, const uint64_t height = 720);

	void Init(const char* title, const uint64_t width, const uint64_t height);
	void Cleanup();

	void CreateVulkanInstance(const char* title);
	void SetupDebugMessenger();

	void PickPhysicalDevice();
	void CreateLogicalDevice();
	VkSampleCountFlagBits GetMaxUsableSampleCount();

	void CreateCommandPool();
	void CreateDescriptorPool();

	void CreateSwapchain();
	void CreateSwapchainImageViews();

	void CreateRenderPass();
	void CreateColorResource();
	void CreateDepthResource();
	void CreateFramebuffers();

	// event callbacks
	void ProcessInput();
	void OnCloseEvent();
	void OnResizeEvent(int width, int height);
	void OnMouseMoveEvent(double xpos, double ypos);
	void OnKeyEvent(int key, int scancode, int action, int mods);

private:
	bool m_IsRunning = true;
	static Engine* s_Instance;
	std::unique_ptr<Window> m_Window;

	VkInstance m_VulkanInstance;
	VkDebugUtilsMessengerEXT m_DebugMessenger;

	QueueFamilyIndices m_QueueFamilyIndices;

	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_DeviceVk;
	VkPhysicalDeviceProperties m_PhysicalDeviceProperties;

	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;

	VkSampleCountFlagBits m_MsaaSamples;

	VkCommandPool m_CommandPool;
	VkDescriptorPool m_DescriptorPool;

	VkSwapchainKHR m_Swapchain;
	std::vector<VkImage> m_SwapchainImages;
	VkFormat m_SwapchainImageFormat;
	VkExtent2D m_SwapchainExtent;
	std::vector<VkImageView> m_SwapchainImageViews;

	VkRenderPass m_RenderPass;

	VkImage m_ColorImage;
	VkDeviceMemory m_ColorImageMemory;
	VkImageView m_ColorImageView;
	VkImage m_DepthImage;
	VkDeviceMemory m_DepthImageMemory;
	VkImageView m_DepthImageView;
	std::vector<VkFramebuffer> m_SwapchainFramebuffers;
};