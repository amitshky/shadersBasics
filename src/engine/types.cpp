#include "engine/types.h"

#include <vulkan/vulkan.h>

#ifdef NDEBUG // release mode
bool VulkanConfig::enableValidationLayers = false;
#else // debug mode
bool VulkanConfig::enableValidationLayers = true;
#endif
uint32_t VulkanConfig::maxFramesInFlight = 2;
std::array<const char*, 1> VulkanConfig::deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
std::array<const char*, 1> VulkanConfig::validationLayers{ "VK_LAYER_KHRONOS_validation" };
