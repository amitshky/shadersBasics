#include "engine/types.h"

#ifdef NDEBUG // release mode
bool Config::enableValidationLayers = false;
#else // debug mode
bool Config::enableValidationLayers = true;
#endif
uint32_t Config::maxFramesInFlight = 2;
std::array<const char*, 1> Config::deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
std::array<const char*, 1> Config::validationLayers{ "VK_LAYER_KHRONOS_validation" };
