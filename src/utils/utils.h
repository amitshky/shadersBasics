#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace utils {

// `VKAPI_ATTR` and `VKAPI_CALL` ensures the right signature for Vulkan
VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbakck,
	void* pUserData);

std::vector<const char*> GetRequiredExtensions();
bool CheckValidationLayerSupport();

} // namespace utils