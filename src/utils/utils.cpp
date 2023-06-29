#include "utils/utils.h"

#include <set>
#include <string>
#include "core/core.h"
#include "core/window.h"
#include "engine/engine.h"

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


// device details functions
bool IsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface)
{
	QueueFamilyIndices indicies = FindQueueFamilies(physicalDevice, windowSurface);

	// checking for extension availability like swapchain extension availability
	bool extensionsSupported = CheckDeviceExtensionSupport(physicalDevice);

	// checking if swapchain is supported by window surface
	bool swapchainAdequate = false;
	if (extensionsSupported)
	{
		SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(physicalDevice, windowSurface);
		swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	return indicies.IsComplete() && extensionsSupported && swapchainAdequate && supportedFeatures.samplerAnisotropy;
}

bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions{ extensionCount };
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions{ VulkanConfig::deviceExtensions.begin(),
		VulkanConfig::deviceExtensions.end() };

	for (const auto& extension : availableExtensions)
		requiredExtensions.erase(extension.extensionName);

	return requiredExtensions.empty();
}

uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
	{
		// typeFilter specifies a bit field of memory types
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	THROW(true, "Failed to find suitable memory type!");
}

SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface)
{
	// Simply checking swapchain availability is not enough,
	// we need to check if it is supported by our window surface or not
	// We need to check for:
	// * basic surface capabilities (min/max number of images in swap chain)
	// * surface formats (pixel format and color space)
	// * available presentation modes
	SwapchainSupportDetails swapchainDetails{};

	// query surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &swapchainDetails.capabilities);

	// query surface format
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		swapchainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			physicalDevice, windowSurface, &formatCount, swapchainDetails.formats.data());
	}

	// query supported presentation modes
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, windowSurface, &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		swapchainDetails.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			physicalDevice, windowSurface, &presentModeCount, swapchainDetails.presentModes.data());
	}

	return swapchainDetails;
}

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies{ queueFamilyCount };
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	// find a queue that supports graphics commands
	for (int i = 0; i < queueFamilies.size(); ++i)
	{
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;

		// check for queue family compatible for presentation
		// the graphics queue and the presentation queue might end up being the
		// same but we treat them as separate queues
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, windowSurface, &presentSupport);

		if (presentSupport)
			indices.presentFamily = i;

		if (indices.IsComplete())
			break;
	}

	return indices;
}

VkFormat FindSupportedFormat(const std::vector<VkFormat>& canditateFormats,
	VkImageTiling tiling,
	VkFormatFeatureFlags features)
{
	for (auto format : canditateFormats)
	{
		VkFormatProperties prop;
		vkGetPhysicalDeviceFormatProperties(Engine::GetPhysicalDevice(), format, &prop);

		if (tiling == VK_IMAGE_TILING_LINEAR && (prop.linearTilingFeatures & features) == features)
			return format;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (prop.optimalTilingFeatures & features) == features)
			return format;
	}

	THROW(true, "Failed to find supported format!");
}


// swapchain
VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& format : availableFormats)
	{
		// NOTE: use `VK_FORMAT_B8G8R8A8_UNORM` for imgui
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;
	}

	return availableFormats[0];
}

VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& presentMode : availablePresentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return presentMode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities,
	std::function<void(int* width, int* height)> pfnGetFramebufferSize)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	else
	{
		int width = 0;
		int height = 0;
		pfnGetFramebufferSize(&width, &height);

		VkExtent2D extent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
		// clamp the values to the allowed range
		extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		extent.height =
			std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return extent;
	}
}

VkFormat FindDepthFormat()
{
	return FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}


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
	VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imgInfo{};
	imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imgInfo.imageType = VK_IMAGE_TYPE_2D;
	imgInfo.extent.width = width;
	imgInfo.extent.height = height;
	imgInfo.extent.depth = 1;
	imgInfo.mipLevels = miplevels;
	imgInfo.arrayLayers = 1;
	imgInfo.format = format;
	imgInfo.tiling = tiling;
	imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imgInfo.usage = usage;
	imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imgInfo.samples = numSamples;
	imgInfo.flags = 0; // for sparse images

	THROW(vkCreateImage(deviceVk, &imgInfo, nullptr, &image) != VK_SUCCESS, "Failed to create image object!")

	VkMemoryRequirements memRequirements{};
	vkGetImageMemoryRequirements(deviceVk, image, &memRequirements);

	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memRequirements.size;
	memAllocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

	THROW(vkAllocateMemory(deviceVk, &memAllocInfo, nullptr, &imageMemory) != VK_SUCCESS,
		"Failed to allocate image memory!")

	vkBindImageMemory(deviceVk, image, imageMemory, 0);
}

VkImageView CreateImageView(VkDevice deviceVk,
	VkImage image,
	VkFormat format,
	VkImageAspectFlags aspectFlags,
	uint32_t miplevels)
{
	VkImageViewCreateInfo imgViewInfo{};
	imgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imgViewInfo.image = image;
	imgViewInfo.format = format;
	imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imgViewInfo.subresourceRange.aspectMask = aspectFlags;
	imgViewInfo.subresourceRange.baseMipLevel = 0;
	imgViewInfo.subresourceRange.levelCount = miplevels;
	imgViewInfo.subresourceRange.baseArrayLayer = 0;
	imgViewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	THROW(vkCreateImageView(deviceVk, &imgViewInfo, nullptr, &imageView) != VK_SUCCESS, "Failed to create image view!")

	return imageView;
}

void CreateBuffer(VkDevice deviceVk,
	VkPhysicalDevice physicalDevice,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	THROW(vkCreateBuffer(deviceVk, &bufferInfo, nullptr, &buffer) != VK_SUCCESS, "Failed to create buffer!")

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(deviceVk, buffer, &memRequirements);

	VkMemoryAllocateInfo allocMemory{};
	allocMemory.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocMemory.allocationSize = memRequirements.size;
	allocMemory.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

	THROW(vkAllocateMemory(deviceVk, &allocMemory, nullptr, &bufferMemory) != VK_SUCCESS, "Failed to allocate memory!")

	vkBindBufferMemory(deviceVk, buffer, bufferMemory, 0);
}

void CopyBuffer(VkDevice deviceVk,
	VkCommandPool commandPool,
	VkQueue graphicsQueue,
	VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	VkDeviceSize size)
{
	VkCommandBuffer cmdBuff = BeginSingleTimeCommands(deviceVk, commandPool);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	// transfer the contents of the buffers
	vkCmdCopyBuffer(cmdBuff, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(cmdBuff, deviceVk, commandPool, graphicsQueue);
}

void CopyBufferToImage(VkDevice deviceVk,
	VkCommandPool commandPool,
	VkQueue graphicsQueue,
	VkBuffer buffer,
	VkImage image,
	uint32_t width,
	uint32_t height)
{
	VkCommandBuffer cmdBuff = BeginSingleTimeCommands(deviceVk, commandPool);

	// specify which part of the buffer is going to be copied to which part of the image
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferImageHeight = 0;
	region.bufferRowLength = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	// part of the image to copy to
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(cmdBuff, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	EndSingleTimeCommands(cmdBuff, deviceVk, commandPool, graphicsQueue);
}

void GenerateMipmaps(VkDevice deviceVk,
	VkPhysicalDevice physicalDevice,
	VkCommandPool commandPool,
	VkQueue graphicsQueue,
	VkImage image,
	VkFormat format,
	int32_t width,
	int32_t height,
	uint32_t mipLevels)
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
	// check image format's support for linear filter
	THROW(!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT),
		"Texture image format does not support linear blitting!");

	VkCommandBuffer cmdBuff = BeginSingleTimeCommands(deviceVk, commandPool);

	VkImageMemoryBarrier imgBarrier{};
	imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imgBarrier.image = image;
	imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgBarrier.subresourceRange.baseArrayLayer = 0;
	imgBarrier.subresourceRange.layerCount = 1;
	imgBarrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = width;
	int32_t mipHeight = height;

	for (uint32_t i = 1; i < mipLevels; ++i)
	{
		// transition the `i - 1` mip level to
		// `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL`
		imgBarrier.subresourceRange.baseMipLevel = i - 1;
		imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imgBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imgBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(cmdBuff,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&imgBarrier);

		// specify the region to be used in blit operation
		// the src mip level is `i - 1`
		// the dst mip level is `i`
		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(cmdBuff,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&blit,
			VK_FILTER_LINEAR);

		// trasition the `i - 1` mip level to
		// `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` all the sampling
		// operations will wait on this transition to finish
		imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imgBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmdBuff,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&imgBarrier);

		// one of the dimensions will reach 1 before the other, so keep it 1
		// when it does (because the image is not a square)
		if (mipWidth > 1)
			mipWidth /= 2;

		if (mipHeight > 1)
			mipHeight /= 2;
	}

	// this barrier transitions the last mip level from
	// `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` to
	// `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` the loop doesnt handle
	// this
	imgBarrier.subresourceRange.baseMipLevel = mipLevels - 1;
	imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imgBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmdBuff,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&imgBarrier);

	EndSingleTimeCommands(cmdBuff, deviceVk, commandPool, graphicsQueue);
}

void TransitionImageLayout(VkDevice deviceVk,
	VkCommandPool commandPool,
	VkQueue graphicsQueue,
	VkImage image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t miplevels)
{
	VkCommandBuffer cmdBuff = BeginSingleTimeCommands(deviceVk, commandPool);

	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = miplevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0; // operation before the barrier
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // operation after the barrier

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		LOG_AND_THROW("Unsupported layout transition!");
	}

	vkCmdPipelineBarrier(cmdBuff, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	EndSingleTimeCommands(cmdBuff, deviceVk, commandPool, graphicsQueue);
}


// commands
VkCommandBuffer BeginSingleTimeCommands(VkDevice deviceVk, VkCommandPool commandPool)
{
	VkCommandBufferAllocateInfo cmdBuffAllocInfo{};
	cmdBuffAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBuffAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBuffAllocInfo.commandPool = commandPool;
	cmdBuffAllocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuff;
	vkAllocateCommandBuffers(deviceVk, &cmdBuffAllocInfo, &cmdBuff);

	// immediately start recording the command buffer
	VkCommandBufferBeginInfo cmdBuffBegin{};
	cmdBuffBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBuffBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmdBuff, &cmdBuffBegin);

	return cmdBuff;
}

void EndSingleTimeCommands(VkCommandBuffer cmdBuff, VkDevice deviceVk, VkCommandPool commandPool, VkQueue graphicsQueue)
{
	vkEndCommandBuffer(cmdBuff);

	// submit the queue
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuff;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(deviceVk, commandPool, 1, &cmdBuff);
}


} // namespace utils