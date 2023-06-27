#include "utils/utils.h"
#include "core/logger.h"
#include "engine/types.h"
#include "core/window.h"

namespace utils {

// the returned value indicates if the Vulkan call that triggered the validation
// layer message should be aborted if true, the call is aborted with
// `VK_ERROR_VALIDATION_FAILED_EXT` error
VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	[[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, // to check the severity of the message
	[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, // contains the actual details of the message
	[[maybe_unused]] void* pUserData) // allows you to pass your own data
{
	Logger::Error("Validation layer: {}\n", pCallbackData->pMessage);
	return VK_FALSE;
}

bool CheckValidationLayerSupport()
{
	// check if all of the requested layers are available
	uint32_t validationLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&validationLayerCount, nullptr);
	std::vector<VkLayerProperties> availableLayerProperites{ validationLayerCount };
	vkEnumerateInstanceLayerProperties(&validationLayerCount, availableLayerProperites.data());

	for (const auto& layer : VulkanConfig::validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayerProperites)
		{
			if (std::strcmp(layer, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
			return false;
	}

	return true;
}

std::vector<const char*> GetRequiredExtensions()
{
	uint32_t extensionCount = 0;
	const char** extensions = Window::GetRequiredVulkanExtensions(&extensionCount);
	std::vector<const char*> availableExtensions{ extensions, extensions + extensionCount };

	if (VulkanConfig::enableValidationLayers)
		availableExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return availableExtensions;
}

} // namespace utils