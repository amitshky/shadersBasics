#pragma once

#include <vector>
#include <functional>
#include <vulkan/vulkan.h>
#include "engine/types.h"

namespace utils {


// `VKAPI_ATTR` and `VKAPI_CALL` ensures the right signature for Vulkan
VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbakck,
	void* pUserData);

std::vector<const char*> GetRequiredExtensions();
bool CheckValidationLayerSupport();


// device details functions
bool IsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface);
bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface);
QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface);
VkFormat FindSupportedFormat(const std::vector<VkFormat>& canditateFormats,
	VkImageTiling tiling,
	VkFormatFeatureFlags features);


// swapchain
VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities,
	std::function<void(int* width, int* height)> pfnGetFramebufferSize);
VkFormat FindDepthFormat();


// images and buffers
void CreateImage(VkDevice deviceVk,
	VkPhysicalDevice physicalDevice,
	uint32_t width,
	uint32_t height,
	uint32_t miplevels,
	VkSampleCountFlagBits numSamples,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkImage& image,
	VkDeviceMemory& imageMemory);

VkImageView CreateImageView(VkDevice deviceVk,
	VkImage image,
	VkFormat format,
	VkImageAspectFlags aspectFlags,
	uint32_t miplevels);

void CreateBuffer(VkDevice deviceVk,
	VkPhysicalDevice physicalDevice,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& bufferMemory);

void CopyBuffer(VkDevice deviceVk,
	VkCommandPool commandPool,
	VkQueue graphicsQueue,
	VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	VkDeviceSize size);

void CopyBufferToImage(VkDevice deviceVk,
	VkCommandPool commandPool,
	VkQueue graphicsQueue,
	VkBuffer buffer,
	VkImage image,
	uint32_t width,
	uint32_t height);

void GenerateMipmaps(VkDevice deviceVk,
	VkPhysicalDevice physicalDevice,
	VkCommandPool commandPool,
	VkQueue graphicsQueue,
	VkImage image,
	VkFormat format,
	int32_t width,
	int32_t height,
	uint32_t mipLevels);

void TransitionImageLayout(VkDevice deviceVk,
	VkCommandPool commandPool,
	VkQueue graphicsQueue,
	VkImage image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t miplevels);


// commands
VkCommandBuffer BeginSingleTimeCommands(VkDevice deviceVk, VkCommandPool commandPool);
void EndSingleTimeCommands(VkCommandBuffer cmdBuff,
	VkDevice deviceVk,
	VkCommandPool commandPool,
	VkQueue graphicsQueue);


} // namespace utils