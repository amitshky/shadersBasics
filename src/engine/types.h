#pragma once

#include <array>
#include <vector>
#include <optional>
#include <vulkan/vulkan.h>

struct VulkanConfig
{
public:
	static bool enableValidationLayers;
	static uint32_t maxFramesInFlight;
	static std::array<const char*, 1> validationLayers;
	static std::array<const char*, 1> deviceExtensions;
};

struct QueueFamilyIndices
{
	// std::optional contains no value until you assign something to it
	// we can check if it contains a value by calling has_value()
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	[[nodiscard]] inline bool IsComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct SwapchainCreateDetails
{
	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
	uint32_t imageCount;
	VkSurfaceKHR windowSurface;
	VkSurfaceTransformFlagBitsKHR currentTransform;
	QueueFamilyIndices queueFamilyIndices;
};