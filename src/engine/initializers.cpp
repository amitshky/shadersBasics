#include "engine/initializers.h"

namespace initializers {


VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo(PFN_vkDebugUtilsMessengerCallbackEXT pfnDebugCallback)
{
	// debug messenger provides explicit control over what kind of messages to log
	VkDebugUtilsMessengerCreateInfoEXT info{};
	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	info.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	//   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
	info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
					   | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
	info.pfnUserCallback = pfnDebugCallback; // call back function for debug messenger
	info.pUserData = nullptr; // Optional

	return info;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
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

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	// must be destroyed before instance is destroyed
	// so we cannot debug any issues in vkDestroyInstance
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, debugMessenger, pAllocator);
}

VkCommandPoolCreateInfo CommandPoolCreateInfo(const QueueFamilyIndices& queueIndices)
{
	VkCommandPoolCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	info.queueFamilyIndex = queueIndices.graphicsFamily.value();

	return info;
}

VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo(VkDescriptorPoolSize* poolSizes, const uint32_t poolSizeCount)
{
	VkDescriptorPoolCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	info.maxSets = 1000;
	info.poolSizeCount = poolSizeCount;
	info.pPoolSizes = poolSizes;

	return info;
}

VkSwapchainCreateInfoKHR SwapchainCreateInfo(const SwapchainCreateDetails& details)
{
	VkSwapchainCreateInfoKHR info{};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = details.windowSurface;
	info.minImageCount = details.imageCount;
	info.imageFormat = details.surfaceFormat.format;
	info.imageColorSpace = details.surfaceFormat.colorSpace;
	info.imageExtent = details.extent;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (details.queueFamilyIndices.graphicsFamily.value() != details.queueFamilyIndices.presentFamily.value())
	{
		uint32_t indicesArr[]{ details.queueFamilyIndices.graphicsFamily.value(),
			details.queueFamilyIndices.presentFamily.value() };
		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices = indicesArr;
	}
	else
	{
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	info.preTransform = details.currentTransform;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode = details.presentMode;
	info.clipped = VK_TRUE;
	info.oldSwapchain = VK_NULL_HANDLE;

	return info;
}

VkAttachmentDescription AttachmentDescription(VkFormat format,
	VkSampleCountFlagBits samples,
	VkImageLayout initialLayout,
	VkImageLayout finalLayout)
{
	VkAttachmentDescription desc{};
	desc.format = format;
	desc.samples = samples;
	desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	desc.initialLayout = initialLayout;
	desc.finalLayout = finalLayout;

	return desc;
}

VkAttachmentReference AttachmentReference(uint32_t attachmentIndex, VkImageLayout layout)
{
	VkAttachmentReference ref{};
	ref.attachment = attachmentIndex;
	ref.layout = layout;

	return ref;
}

VkSubpassDescription SubpassDescription(uint32_t colorAttachmentCount,
	const VkAttachmentReference* pColorAttachments,
	const VkAttachmentReference* pDepthStencilAttachment,
	const VkAttachmentReference* pResolveAttachments)
{
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = colorAttachmentCount;
	subpass.pColorAttachments = pColorAttachments;
	subpass.pResolveAttachments = pResolveAttachments;
	subpass.pDepthStencilAttachment = pDepthStencilAttachment;

	return subpass;
}

VkSubpassDependency SubpassDependency(uint32_t srcSubpass,
	uint32_t dstSubpass,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask,
	VkAccessFlags srcAccessMask,
	VkAccessFlags dstAccessMask)
{
	VkSubpassDependency dep{};
	dep.srcSubpass = srcSubpass;
	dep.dstSubpass = dstSubpass;
	dep.srcStageMask = srcStageMask;
	dep.dstStageMask = dstStageMask;
	dep.srcAccessMask = srcAccessMask;
	dep.dstAccessMask = dstAccessMask;

	return dep;
}

VkRenderPassCreateInfo RenderPassCreateInfo(uint32_t attachmentCount,
	const VkAttachmentDescription* pAttachments,
	uint32_t subpassCount,
	const VkSubpassDescription* pSubpasses,
	uint32_t dependencyCount,
	const VkSubpassDependency* pDependencies)
{
	VkRenderPassCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = attachmentCount;
	info.pAttachments = pAttachments;
	info.subpassCount = subpassCount;
	info.pSubpasses = pSubpasses;
	info.dependencyCount = dependencyCount;
	info.pDependencies = pDependencies;

	return info;
}


} // namespace initializers