#include "engine/engine.h"

#include <set>
#include "core/core.h"
#include "core/input.h"
#include "engine/initializers.h"
#include "utils/utils.h"


Engine* Engine::s_Instance = nullptr;

Engine::Engine(const char* title, const uint64_t width, const uint64_t height)
{
	s_Instance = this;
	Init(title, width, height);
}

Engine::~Engine()
{
	Cleanup();
}

Engine* Engine::Create(const char* title, const uint64_t width, const uint64_t height)
{
	if (s_Instance == nullptr)
		return new Engine(title, width, height);

	return s_Instance;
}

void Engine::Init(const char* title, const uint64_t width, const uint64_t height)
{
	m_Window = std::make_unique<Window>(WindowProps{ title, width, height });
	// set window event callbacks
	m_Window->SetCloseEventCallbackFn(BIND_FN(Engine::OnCloseEvent));
	m_Window->SetResizeEventCallbackFn(BIND_FN(Engine::OnResizeEvent));
	m_Window->SetMouseEventCallbackFn(BIND_FN(Engine::OnMouseMoveEvent));
	m_Window->SetKeyEventCallbackFn(BIND_FN(Engine::OnKeyEvent));

	Logger::Info("{} application initialized!", title);

	CreateVulkanInstance(title);
	SetupDebugMessenger();
	m_Window->CreateWindowSurface(m_VulkanInstance);

	PickPhysicalDevice();
	CreateLogicalDevice();

	CreateCommandPool();
	CreateDescriptorPool();

	CreateSwapchain();
	CreateSwapchainImageViews();
	CreateRenderPass();
	CreateColorResource();
	CreateDepthResource();
	CreateFramebuffers();
}

void Engine::Cleanup()
{
	vkDestroyImageView(m_DeviceVk, m_DepthImageView, nullptr);
	vkDestroyImage(m_DeviceVk, m_DepthImage, nullptr);
	vkFreeMemory(m_DeviceVk, m_DepthImageMemory, nullptr);

	vkDestroyImageView(m_DeviceVk, m_ColorImageView, nullptr);
	vkDestroyImage(m_DeviceVk, m_ColorImage, nullptr);
	vkFreeMemory(m_DeviceVk, m_ColorImageMemory, nullptr);

	for (const auto& framebuffer : m_SwapchainFramebuffers)
		vkDestroyFramebuffer(m_DeviceVk, framebuffer, nullptr);

	vkDestroyRenderPass(m_DeviceVk, m_RenderPass, nullptr);

	for (const auto& imageView : m_SwapchainImageViews)
		vkDestroyImageView(m_DeviceVk, imageView, nullptr);

	// swapchain images are destroyed with `vkDestroySwapchainKHR()`
	vkDestroySwapchainKHR(m_DeviceVk, m_Swapchain, nullptr);

	vkDestroyDescriptorPool(m_DeviceVk, m_DescriptorPool, nullptr);
	vkDestroyCommandPool(m_DeviceVk, m_CommandPool, nullptr);

	vkDestroyDevice(m_DeviceVk, nullptr);

	m_Window->DestroyWindowSurface(m_VulkanInstance);
	if (VulkanConfig::enableValidationLayers)
		initializers::DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);
	vkDestroyInstance(m_VulkanInstance, nullptr);
}

void Engine::Run()
{
	while (m_IsRunning)
	{
		ProcessInput();
		m_Window->OnUpdate();
	}
}

void Engine::CreateVulkanInstance(const char* title)
{
	THROW(VulkanConfig::enableValidationLayers && !utils::CheckValidationLayerSupport(),
		"Validation layers requested, but not available!")

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = title;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0; // the highest version the application is designed to use

	VkInstanceCreateInfo instanceInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	// get required extensions
	std::vector<const char*> extensions = utils::GetRequiredExtensions();
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceInfo.ppEnabledExtensionNames = extensions.data();

	// specify global validation layers
	if (VulkanConfig::enableValidationLayers)
	{
		instanceInfo.enabledLayerCount = static_cast<uint32_t>(VulkanConfig::validationLayers.size());
		instanceInfo.ppEnabledLayerNames = VulkanConfig::validationLayers.data();

		// debug messenger
		VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo =
			initializers::DebugMessengerCreateInfo(utils::DebugCallback);
		instanceInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugMessengerInfo);
	}
	else
	{
		instanceInfo.enabledLayerCount = 0;
		instanceInfo.pNext = nullptr;
	}

	THROW(
		vkCreateInstance(&instanceInfo, nullptr, &m_VulkanInstance) != VK_SUCCESS, "Failed to create Vulkan instance!")
}

void Engine::SetupDebugMessenger()
{
	if (!VulkanConfig::enableValidationLayers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo =
		initializers::DebugMessengerCreateInfo(utils::DebugCallback);

	THROW(initializers::CreateDebugUtilsMessengerEXT(m_VulkanInstance, &debugMessengerInfo, nullptr, &m_DebugMessenger)
			  != VK_SUCCESS,
		"Failed to setup debug messenger!");
}

void Engine::PickPhysicalDevice()
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDeviceCount, nullptr);

	THROW(physicalDeviceCount == 0, "Failed to find GPUs with Vulkan support!")

	std::vector<VkPhysicalDevice> physicalDevices{ physicalDeviceCount };
	vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDeviceCount, physicalDevices.data());

	for (const auto& device : physicalDevices)
	{
		if (utils::IsDeviceSuitable(device, m_Window->GetWindowSurface()))
		{
			m_PhysicalDevice = device;
			vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalDeviceProperties);
			m_MsaaSamples = GetMaxUsableSampleCount();
			break;
		}
	}

	THROW(m_PhysicalDevice == VK_NULL_HANDLE, "Failed to find a suitable GPU!")

	Logger::Info(
		"Physical device info:\n"
		"    Device name: {}",
		m_PhysicalDeviceProperties.deviceName);
}

void Engine::CreateLogicalDevice()
{
	// create queue
	m_QueueFamilyIndices = utils::FindQueueFamilies(m_PhysicalDevice, m_Window->GetWindowSurface());

	// we have multiple queues so we create a set of unique queue families
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
	std::set<uint32_t> uniqueQueueFamilies = { m_QueueFamilyIndices.graphicsFamily.value(),
		m_QueueFamilyIndices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (const auto& queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = queueFamily;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueInfo);
	}

	// specify used device features
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE; // enable sample shading

	// create logical device
	VkDeviceCreateInfo deviceInfo{};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceInfo.pEnabledFeatures = &deviceFeatures;

	// these are similar to create instance but they are device specific this
	// time
	deviceInfo.enabledExtensionCount = static_cast<uint32_t>(VulkanConfig::deviceExtensions.size());
	deviceInfo.ppEnabledExtensionNames = VulkanConfig::deviceExtensions.data();

	if (VulkanConfig::enableValidationLayers)
	{
		deviceInfo.enabledLayerCount = static_cast<uint32_t>(VulkanConfig::validationLayers.size());
		deviceInfo.ppEnabledLayerNames = VulkanConfig::validationLayers.data();
	}
	else
	{
		deviceInfo.enabledLayerCount = 0;
	}

	THROW(vkCreateDevice(m_PhysicalDevice, &deviceInfo, nullptr, &m_DeviceVk) != VK_SUCCESS,
		"Failed to create logcial device!");

	// get the queue handle
	vkGetDeviceQueue(m_DeviceVk, m_QueueFamilyIndices.graphicsFamily.value(), 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_DeviceVk, m_QueueFamilyIndices.presentFamily.value(), 0, &m_PresentQueue);
}

VkSampleCountFlagBits Engine::GetMaxUsableSampleCount()
{
	VkSampleCountFlags counts = m_PhysicalDeviceProperties.limits.framebufferColorSampleCounts
								& m_PhysicalDeviceProperties.limits.framebufferDepthSampleCounts;

	if (counts & VK_SAMPLE_COUNT_64_BIT)
		return VK_SAMPLE_COUNT_64_BIT;

	if (counts & VK_SAMPLE_COUNT_32_BIT)
		return VK_SAMPLE_COUNT_32_BIT;

	if (counts & VK_SAMPLE_COUNT_16_BIT)
		return VK_SAMPLE_COUNT_16_BIT;

	if (counts & VK_SAMPLE_COUNT_8_BIT)
		return VK_SAMPLE_COUNT_8_BIT;

	if (counts & VK_SAMPLE_COUNT_4_BIT)
		return VK_SAMPLE_COUNT_4_BIT;

	if (counts & VK_SAMPLE_COUNT_2_BIT)
		return VK_SAMPLE_COUNT_2_BIT;

	return VK_SAMPLE_COUNT_1_BIT;
}

void Engine::CreateCommandPool()
{
	VkCommandPoolCreateInfo commandPoolInfo = initializers::CommandPoolCreateInfo(m_QueueFamilyIndices);
	THROW(vkCreateCommandPool(m_DeviceVk, &commandPoolInfo, nullptr, &m_CommandPool) != VK_SUCCESS,
		"Failed to create command pool!")
}

void Engine::CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSizes[] = {
		{VK_DESCRIPTOR_TYPE_SAMPLER,                 1000},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000},
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000},
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000},
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000},
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo =
		initializers::DescriptorPoolCreateInfo(poolSizes, std::size(poolSizes));
	THROW(vkCreateDescriptorPool(m_DeviceVk, &descriptorPoolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS,
		"Failed to create descriptor pool!");
}

void Engine::CreateSwapchain()
{
	SwapchainSupportDetails swapchainSupport =
		utils::QuerySwapchainSupport(m_PhysicalDevice, m_Window->GetWindowSurface());
	VkSurfaceFormatKHR surfaceFormat = utils::ChooseSurfaceFormat(swapchainSupport.formats);
	VkPresentModeKHR presentMode = utils::ChoosePresentMode(swapchainSupport.presentModes);
	VkExtent2D extent = utils::ChooseExtent(swapchainSupport.capabilities, BIND_FN(m_Window->GetFramebufferSize));

	uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
	if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
		imageCount = swapchainSupport.capabilities.maxImageCount;

	SwapchainCreateDetails swapchainDetails{};
	swapchainDetails.surfaceFormat = surfaceFormat;
	swapchainDetails.presentMode = presentMode;
	swapchainDetails.extent = extent;
	swapchainDetails.imageCount = imageCount;
	swapchainDetails.windowSurface = m_Window->GetWindowSurface();
	swapchainDetails.currentTransform = swapchainSupport.capabilities.currentTransform;
	swapchainDetails.queueFamilyIndices = m_QueueFamilyIndices;

	VkSwapchainCreateInfoKHR swapchainInfo = initializers::SwapchainCreateInfo(swapchainDetails);
	THROW(vkCreateSwapchainKHR(m_DeviceVk, &swapchainInfo, nullptr, &m_Swapchain) != VK_SUCCESS,
		"Failed to create swapchain!")

	vkGetSwapchainImagesKHR(m_DeviceVk, m_Swapchain, &imageCount, nullptr);
	m_SwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_DeviceVk, m_Swapchain, &imageCount, m_SwapchainImages.data());

	m_SwapchainImageFormat = surfaceFormat.format;
	m_SwapchainExtent = extent;
}

void Engine::CreateSwapchainImageViews()
{
	m_SwapchainImageViews.resize(m_SwapchainImages.size());

	for (size_t i = 0; i < m_SwapchainImageViews.size(); ++i)
	{
		m_SwapchainImageViews[i] = utils::CreateImageView(
			m_DeviceVk, m_SwapchainImages[i], m_SwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

void Engine::CreateRenderPass()
{
	VkFormat depthFormat = utils::FindDepthFormat();

	// color attachment description
	VkAttachmentDescription colorAttachment = initializers::AttachmentDescription(
		m_SwapchainImageFormat, m_MsaaSamples, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	// depth attachment description
	VkAttachmentDescription depthAttachment = initializers::AttachmentDescription(
		depthFormat, m_MsaaSamples, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	// color resolve attachment description (Multisample)
	VkAttachmentDescription colorResolveAttachment = initializers::AttachmentDescription(
		m_SwapchainImageFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// attachment refrences
	VkAttachmentReference colorRef =
		initializers::AttachmentReference(0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	VkAttachmentReference depthRef =
		initializers::AttachmentReference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	VkAttachmentReference colorResolveRef =
		initializers::AttachmentReference(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// subpass
	VkSubpassDescription subpass = initializers::SubpassDescription(1, &colorRef, &depthRef, &colorResolveRef);
	// subpass dependency
	VkSubpassDependency subpassDependency = initializers::SubpassDependency(VK_SUBPASS_EXTERNAL,
		0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		0,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
	std::array<VkAttachmentDescription, 3> attachments{ colorAttachment, depthAttachment, colorResolveAttachment };

	// render pass
	VkRenderPassCreateInfo renderPassInfo = initializers::RenderPassCreateInfo(
		static_cast<uint32_t>(attachments.size()), attachments.data(), 1, &subpass, 1, &subpassDependency);
	THROW(vkCreateRenderPass(m_DeviceVk, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS,
		"Failed to create render pass!");
}

void Engine::CreateColorResource()
{
	VkFormat colorFormat = m_SwapchainImageFormat;
	uint32_t miplevels = 1;

	utils::CreateImage(m_DeviceVk,
		m_PhysicalDevice,
		m_SwapchainExtent.width,
		m_SwapchainExtent.height,
		miplevels,
		m_MsaaSamples,
		colorFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_ColorImage,
		m_ColorImageMemory);

	m_ColorImageView =
		utils::CreateImageView(m_DeviceVk, m_ColorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, miplevels);
}

void Engine::CreateDepthResource()
{
	VkFormat depthFormat = utils::FindDepthFormat();
	uint32_t miplevels = 1;

	utils::CreateImage(m_DeviceVk,
		m_PhysicalDevice,
		m_SwapchainExtent.width,
		m_SwapchainExtent.height,
		miplevels,
		m_MsaaSamples,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_DepthImage,
		m_DepthImageMemory);

	m_DepthImageView =
		utils::CreateImageView(m_DeviceVk, m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, miplevels);
}

void Engine::CreateFramebuffers()
{
	m_SwapchainFramebuffers.resize(m_SwapchainImages.size());

	for (size_t i = 0; i < m_SwapchainImages.size(); ++i)
	{
		std::array<VkImageView, 3> fbAttachments{ m_ColorImageView, m_DepthImageView, m_SwapchainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_RenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
		framebufferInfo.pAttachments = fbAttachments.data();
		framebufferInfo.width = m_SwapchainExtent.width;
		framebufferInfo.height = m_SwapchainExtent.height;
		framebufferInfo.layers = 1;

		THROW(vkCreateFramebuffer(m_DeviceVk, &framebufferInfo, nullptr, &m_SwapchainFramebuffers[i]) != VK_SUCCESS,
			"Failed to create framebuffer!")
	}
}


// event callbacks
void Engine::ProcessInput()
{
	// forward input data to ImGui first
	// ImGuiIO& io = ImGui::GetIO();
	// if (io.WantCaptureMouse || io.WantCaptureKeyboard)
	// 	return;

	if (Input::IsMouseButtonPressed(Mouse::BUTTON_1))
	{
		// hide cursor when moving camera
		m_Window->HideCursor();
	}
	else if (Input::IsMouseButtonReleased(Mouse::BUTTON_1))
	{
		// unhide cursor when camera stops moving
		m_Window->ShowCursor();
	}
}

void Engine::OnCloseEvent()
{
	m_IsRunning = false;
}

void Engine::OnResizeEvent(int width, int height)
{}

void Engine::OnMouseMoveEvent(double xpos, double ypos)
{
	// ImGuiIO& io = ImGui::GetIO();
	// if (io.WantCaptureMouse)
	// 	return;
}

void Engine::OnKeyEvent(int key, int scancode, int action, int mods)
{
	// quits the application
	// works even when the ui is in focus
	if (Input::IsKeyPressed(Key::LEFT_CONTROL) && Input::IsKeyPressed(Key::Q))
		m_IsRunning = false;

	// 	ImGuiIO& io = ImGui::GetIO();
	// 	if (io.WantCaptureKeyboard)
	// 		return;
}