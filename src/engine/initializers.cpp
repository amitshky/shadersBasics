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

VkFramebufferCreateInfo FramebufferCreateInfo(VkRenderPass renderPass,
	uint32_t attachmentCount,
	const VkImageView* pAttachments,
	uint32_t width,
	uint32_t height)
{
	VkFramebufferCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass = renderPass;
	info.attachmentCount = attachmentCount;
	info.pAttachments = pAttachments;
	info.width = width;
	info.height = height;
	info.layers = 1;

	return info;
}


// render pass
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


// descriptors
VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo(uint32_t setLayoutCount,
	const VkDescriptorSetLayout* pSetLayouts,
	uint32_t pushConstantRangeCount,
	const VkPushConstantRange* pPushConstantRanges)
{
	VkPipelineLayoutCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.setLayoutCount = setLayoutCount;
	info.pSetLayouts = pSetLayouts;
	info.pushConstantRangeCount = pushConstantRangeCount;
	info.pPushConstantRanges = pPushConstantRanges;

	return info;
}

VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(uint32_t binding,
	VkDescriptorType descriptorType,
	uint32_t descriptorCount,
	VkShaderStageFlags stageFlags)
{
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = binding;
	layoutBinding.descriptorType = descriptorType;
	layoutBinding.descriptorCount = descriptorCount;
	layoutBinding.stageFlags = stageFlags;
	layoutBinding.pImmutableSamplers = nullptr;

	return layoutBinding;
}

VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo(uint32_t bindingCount,
	const VkDescriptorSetLayoutBinding* pBindings)
{
	VkDescriptorSetLayoutCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.bindingCount = bindingCount;
	info.pBindings = pBindings;
	return info;
	;
}

VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo(VkDescriptorPool descriptorPool,
	uint32_t descriptorSetCount,
	const VkDescriptorSetLayout* pSetLayouts)
{
	VkDescriptorSetAllocateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info.descriptorPool = descriptorPool;
	info.descriptorSetCount = descriptorSetCount;
	info.pSetLayouts = pSetLayouts;

	return info;
}

VkDescriptorBufferInfo DescriptorBufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
{
	VkDescriptorBufferInfo info{};
	info.buffer = buffer;
	info.offset = offset;
	info.range = range;

	return info;
}

VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorSet dstSet,
	uint32_t dstBinding,
	VkDescriptorType descriptorType,
	uint32_t descriptorCount,
	const VkDescriptorBufferInfo* pBufferInfo,
	const VkDescriptorImageInfo* pImageInfo)
{
	VkWriteDescriptorSet descWrite{};
	descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descWrite.dstSet = dstSet;
	descWrite.dstBinding = dstBinding;
	descWrite.dstArrayElement = 0;
	descWrite.descriptorType = descriptorType;
	descWrite.descriptorCount = descriptorCount;
	descWrite.pBufferInfo = pBufferInfo;
	descWrite.pImageInfo = pImageInfo;

	return descWrite;
}


// pipeline
VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo(uint32_t vertexBindingDescriptionCount,
	const VkVertexInputBindingDescription* pVertexBindingDescriptions,
	uint32_t vertexAttributeDescriptionCount,
	const VkVertexInputAttributeDescription* pVertexAttributeDescriptions)
{
	VkPipelineVertexInputStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	info.vertexBindingDescriptionCount = vertexBindingDescriptionCount;
	info.pVertexBindingDescriptions = pVertexBindingDescriptions;
	info.vertexAttributeDescriptionCount = vertexAttributeDescriptionCount;
	info.pVertexAttributeDescriptions = pVertexAttributeDescriptions;

	return info;
}

VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology)
{
	VkPipelineInputAssemblyStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	info.topology = topology;
	info.primitiveRestartEnable = VK_FALSE;

	return info;
}

VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo(uint32_t viewportCount, uint32_t scissorCount)
{
	VkPipelineViewportStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	info.viewportCount = 1;
	info.scissorCount = 1;

	return info;
}

VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo(VkCullModeFlags cullMode,
	VkFrontFace frontFace)
{
	VkPipelineRasterizationStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	info.depthClampEnable = VK_FALSE;
	info.rasterizerDiscardEnable = VK_FALSE;
	info.polygonMode = VK_POLYGON_MODE_FILL;
	info.lineWidth = 1.0f;
	info.cullMode = cullMode;
	info.frontFace = frontFace;
	info.depthBiasEnable = VK_FALSE;
	info.depthBiasConstantFactor = 0.0f;
	info.depthBiasClamp = 0.0f;
	info.depthBiasSlopeFactor = 0.0f;

	return info;
}

VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo(VkBool32 sampleShadingEnable,
	VkSampleCountFlagBits rasterizationSamples,
	float minSampleShading)
{
	VkPipelineMultisampleStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	info.sampleShadingEnable = sampleShadingEnable;
	info.rasterizationSamples = rasterizationSamples;
	info.minSampleShading = minSampleShading; // closer to 1 is smoother
	info.pSampleMask = nullptr;
	info.alphaToCoverageEnable = VK_FALSE;
	info.alphaToOneEnable = VK_FALSE;

	return info;
}

VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo(VkBool32 depthTestEnable,
	VkBool32 depthWriteEnable)
{
	VkPipelineDepthStencilStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	info.depthTestEnable = depthTestEnable;
	info.depthWriteEnable = depthWriteEnable;
	info.depthCompareOp = VK_COMPARE_OP_LESS;
	info.depthBoundsTestEnable = VK_FALSE;
	info.minDepthBounds = 0.0f;
	info.maxDepthBounds = 1.0f;
	info.stencilTestEnable = VK_FALSE;
	info.front = {};
	info.back = {};

	return info;
}

VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo(
	const VkPipelineColorBlendAttachmentState& colorBlendAttachment)
{
	VkPipelineColorBlendStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	info.logicOpEnable = VK_FALSE;
	info.logicOp = VK_LOGIC_OP_COPY;
	info.attachmentCount = 1;
	info.pAttachments = &colorBlendAttachment;
	info.blendConstants[0] = 0.0f;
	info.blendConstants[1] = 0.0f;
	info.blendConstants[2] = 0.0f;
	info.blendConstants[3] = 0.0f;

	return info;
}

VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo(uint32_t dynamicStateCount,
	const VkDynamicState* pDynamicStates)
{
	VkPipelineDynamicStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	info.dynamicStateCount = dynamicStateCount;
	info.pDynamicStates = pDynamicStates;

	return info;
}


// command buffer
VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool commandPool, uint32_t commandBufferCount)
{
	VkCommandBufferAllocateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandPool = commandPool;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount = commandBufferCount;

	return info;
}


} // namespace initializers