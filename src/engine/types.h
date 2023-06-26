#pragma once

#include <array>

struct VulkanConfig
{
public:
	static bool enableValidationLayers;
	static uint32_t maxFramesInFlight;
	static std::array<const char*, 1> validationLayers;
	static std::array<const char*, 1> deviceExtensions;
};