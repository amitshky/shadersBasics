#pragma once

#include <vulkan/vulkan.h>
#include "engine/types.h"

namespace initializers {

VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo(PFN_vkDebugUtilsMessengerCallbackEXT pfnDebugCallback);

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator);

VkCommandPoolCreateInfo CommandPoolCreateInfo(const QueueFamilyIndices& queueIndices);

VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo(VkDescriptorPoolSize* poolSizes, const uint32_t poolSizeCount);

VkSwapchainCreateInfoKHR SwapchainCreateInfo(const SwapchainCreateDetails& details);

// render pass
VkAttachmentDescription AttachmentDescription(VkFormat format,
	VkSampleCountFlagBits samples,
	VkImageLayout initialLayout,
	VkImageLayout finalLayout);
VkAttachmentReference AttachmentReference(uint32_t attachmentIndex, VkImageLayout layout);
VkSubpassDescription SubpassDescription(uint32_t colorAttachmentCount,
	const VkAttachmentReference* pColorAttachments,
	const VkAttachmentReference* pDepthStencilAttachment,
	const VkAttachmentReference* pResolveAttachments);
VkSubpassDependency SubpassDependency(uint32_t srcSubpass,
	uint32_t dstSubpass,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask,
	VkAccessFlags srcAccessMask,
	VkAccessFlags dstAccessMask);
VkRenderPassCreateInfo RenderPassCreateInfo(uint32_t attachmentCount,
	const VkAttachmentDescription* pAttachments,
	uint32_t subpassCount,
	const VkSubpassDescription* pSubpasses,
	uint32_t dependencyCount,
	const VkSubpassDependency* pDependencies);


} // namespace initializers