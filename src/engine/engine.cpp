#include "engine/engine.h"

#include <set>
#include "core/core.h"
#include "core/input.h"
#include "engine/initializers.h"
#include "engine/shader.h"
#include "ui/imGuiOverlay.h"
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

	CreatePipelineLayout();
	CreatePipeline("assets/shaders/shader.vert.spv", "assets/shaders/shader.frag.spv");

	CreateCommandBuffers();

	CreateSyncObjects();

	ImGuiOverlay::Init(m_VulkanInstance,
		m_PhysicalDevice,
		m_DeviceVk,
		m_QueueFamilyIndices.graphicsFamily.value(),
		m_GraphicsQueue,
		m_MsaaSamples,
		m_RenderPass,
		m_CommandPool,
		Config::maxFramesInFlight);
}

void Engine::Cleanup()
{
	vkDeviceWaitIdle(m_DeviceVk);

	ImGuiOverlay::Cleanup(m_DeviceVk);

	for (size_t i = 0; i < Config::maxFramesInFlight; ++i)
	{
		vkDestroySemaphore(m_DeviceVk, m_ImageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_DeviceVk, m_RenderFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_DeviceVk, m_InFlightFences[i], nullptr);
	}

	vkDestroyPipeline(m_DeviceVk, m_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_DeviceVk, m_PipelineLayout, nullptr);

	CleanupSwapchain();
	vkDestroyRenderPass(m_DeviceVk, m_RenderPass, nullptr);

	vkDestroyDescriptorPool(m_DeviceVk, m_DescriptorPool, nullptr);
	vkDestroyCommandPool(m_DeviceVk, m_CommandPool, nullptr);

	vkDestroyDevice(m_DeviceVk, nullptr);

	m_Window->DestroyWindowSurface(m_VulkanInstance);
	if (Config::enableValidationLayers)
		initializers::DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);
	vkDestroyInstance(m_VulkanInstance, nullptr);
}

void Engine::Run()
{
	m_LastFrameTime = std::chrono::high_resolution_clock::now();
	while (m_IsRunning)
	{
		float deltatime = CalcFps();
		Draw();
		ProcessInput();
		m_Window->OnUpdate();
	}
}

void Engine::Draw()
{
	BeginScene();

	vkCmdBindPipeline(m_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	vkCmdDraw(m_ActiveCommandBuffer, 6, 1, 0, 0);

	OnUiRender();
	EndScene();
}

void Engine::BeginScene()
{
	// wait for previous frame to signal the fence
	vkWaitForFences(m_DeviceVk, 1, &m_InFlightFences[m_CurrentFrameIndex], VK_TRUE, UINT64_MAX);

	VkResult result = vkAcquireNextImageKHR(m_DeviceVk,
		m_Swapchain,
		UINT64_MAX,
		m_ImageAvailableSemaphores[m_CurrentFrameIndex],
		VK_NULL_HANDLE,
		&m_NextFrameIndex);
	THROW(result != VK_SUCCESS, "Failed to acquire swapchain image!")

	// resetting the fence has been set after the result has been checked to
	// avoid a deadlock reset the fence to unsignaled state
	vkResetFences(m_DeviceVk, 1, &m_InFlightFences[m_CurrentFrameIndex]);

	// begin command buffer
	m_ActiveCommandBuffer = m_CommandBuffers[m_CurrentFrameIndex];
	vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrameIndex], 0);
	VkCommandBufferBeginInfo cmdBuffBeginInfo{};
	cmdBuffBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	THROW(vkBeginCommandBuffer(m_CommandBuffers[m_CurrentFrameIndex], &cmdBuffBeginInfo) != VK_SUCCESS,
		"Failed to begin recording command buffer!")

	// begin render pass
	// clear values for each attachment
	std::array<VkClearValue, 3> clearValues;
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	clearValues[2].color = clearValues[0].color;
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_SwapchainExtent.width);
	viewport.height = static_cast<float>(m_SwapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(m_ActiveCommandBuffer, 0, 1, &viewport);
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_SwapchainExtent;
	vkCmdSetScissor(m_ActiveCommandBuffer, 0, 1, &scissor);

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_RenderPass;
	renderPassBeginInfo.framebuffer = m_SwapchainFramebuffers[m_NextFrameIndex];
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = m_SwapchainExtent;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(m_ActiveCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Engine::EndScene()
{
	vkCmdEndRenderPass(m_ActiveCommandBuffer);
	THROW(vkEndCommandBuffer(m_CommandBuffers[m_CurrentFrameIndex]) != VK_SUCCESS, "Failed to record command buffer!");

	std::array<VkPipelineStageFlags, 1> waitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrameIndex];
	submitInfo.pWaitDstStageMask = waitStages.data();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_CommandBuffers[m_CurrentFrameIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrameIndex];

	// signals the fence after executing the command buffer
	THROW(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrameIndex]) != VK_SUCCESS,
		"Failed to submit draw command buffer!")

	std::array<VkSwapchainKHR, 1> swapchains{ m_Swapchain };
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrameIndex];
	presentInfo.swapchainCount = static_cast<uint32_t>(swapchains.size());
	presentInfo.pSwapchains = swapchains.data();
	presentInfo.pImageIndices = &m_NextFrameIndex;
	presentInfo.pResults = nullptr;

	vkQueuePresentKHR(m_PresentQueue, &presentInfo);

	// update current frame index
	m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % Config::maxFramesInFlight;
}

void Engine::OnUiRender()
{
	ImGuiOverlay::Begin();

	ImGui::Begin("Profiler");
	ImGui::Text("%.2f ms/frame (%d fps)", (1000.0f / m_LastFps), m_LastFps);
	ImGui::End();

	ImGuiOverlay::End(m_ActiveCommandBuffer);
}

float Engine::CalcFps()
{
	++m_FrameCounter;
	std::chrono::time_point<std::chrono::high_resolution_clock> currentFrameTime =
		std::chrono::high_resolution_clock::now();

	float deltatime =
		std::chrono::duration<float, std::chrono::milliseconds::period>(currentFrameTime - m_LastFrameTime).count();
	m_LastFrameTime = currentFrameTime;

	float fpsTimer =
		std::chrono::duration<float, std::chrono::milliseconds::period>(currentFrameTime - m_FpsTimePoint).count();
	// calc fps every 1000ms
	if (fpsTimer > 1000.0f)
	{
		m_LastFps = static_cast<uint32_t>(static_cast<float>(m_FrameCounter) * (1000.0f / fpsTimer));
		m_FrameCounter = 0;
		m_FpsTimePoint = currentFrameTime;
	}

	return deltatime;
}

void Engine::CreateVulkanInstance(const char* title)
{
	THROW(Config::enableValidationLayers && !utils::CheckValidationLayerSupport(),
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
	if (Config::enableValidationLayers)
	{
		instanceInfo.enabledLayerCount = static_cast<uint32_t>(Config::validationLayers.size());
		instanceInfo.ppEnabledLayerNames = Config::validationLayers.data();

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
	if (!Config::enableValidationLayers)
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
	deviceInfo.enabledExtensionCount = static_cast<uint32_t>(Config::deviceExtensions.size());
	deviceInfo.ppEnabledExtensionNames = Config::deviceExtensions.data();

	if (Config::enableValidationLayers)
	{
		deviceInfo.enabledLayerCount = static_cast<uint32_t>(Config::validationLayers.size());
		deviceInfo.ppEnabledLayerNames = Config::validationLayers.data();
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

void Engine::RecreateSwapchain()
{
	while (m_Window->IsMinimized())
	{
		m_Window->WaitEvents();
	}

	vkDeviceWaitIdle(m_DeviceVk);
	CleanupSwapchain();

	CreateSwapchain();
	CreateSwapchainImageViews();
	CreateColorResource();
	CreateDepthResource();
	CreateFramebuffers();
}

void Engine::CleanupSwapchain()
{
	vkDestroyImageView(m_DeviceVk, m_DepthImageView, nullptr);
	vkDestroyImage(m_DeviceVk, m_DepthImage, nullptr);
	vkFreeMemory(m_DeviceVk, m_DepthImageMemory, nullptr);

	vkDestroyImageView(m_DeviceVk, m_ColorImageView, nullptr);
	vkDestroyImage(m_DeviceVk, m_ColorImage, nullptr);
	vkFreeMemory(m_DeviceVk, m_ColorImageMemory, nullptr);

	for (const auto& framebuffer : m_SwapchainFramebuffers)
		vkDestroyFramebuffer(m_DeviceVk, framebuffer, nullptr);

	for (const auto& imageView : m_SwapchainImageViews)
		vkDestroyImageView(m_DeviceVk, imageView, nullptr);

	// swapchain images are destroyed with `vkDestroySwapchainKHR()`
	vkDestroySwapchainKHR(m_DeviceVk, m_Swapchain, nullptr);
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
	VkAttachmentReference colorRef = initializers::AttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
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

void Engine::CreatePipelineLayout()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = initializers::PipelineLayoutCreateInfo();
	THROW(vkCreatePipelineLayout(m_DeviceVk, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS,
		"Failed to create pipeline layout!");
}

void Engine::CreatePipeline(const char* vertShaderPath, const char* fragShaderPath)
{
	// shader stages
	Shader vertexShader{ m_DeviceVk, vertShaderPath, ShaderType::VERTEX };
	Shader fragmentShader{ m_DeviceVk, fragShaderPath, ShaderType::FRAGMENT };
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{ vertexShader.GetShaderStage(),
		fragmentShader.GetShaderStage() };

	// fixed functions
	// vertex input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	// input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	// viewport state
	VkPipelineViewportStateCreateInfo viewportStateInfo{};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.scissorCount = 1;

	// rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizationStateInfo{};
	rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateInfo.lineWidth = 1.0f;
	rasterizationStateInfo.cullMode = VK_CULL_MODE_NONE;
	rasterizationStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateInfo.depthBiasClamp = 0.0f;
	rasterizationStateInfo.depthBiasSlopeFactor = 0.0f;

	// multisampling
	VkPipelineMultisampleStateCreateInfo multisampleStateInfo{};
	multisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateInfo.rasterizationSamples = m_MsaaSamples;
	multisampleStateInfo.sampleShadingEnable = VK_TRUE;
	multisampleStateInfo.minSampleShading = 0.2f; // min fraction for sample shading; closer to 1 is smoother
	multisampleStateInfo.pSampleMask = nullptr;
	multisampleStateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateInfo.alphaToOneEnable = VK_FALSE;

	// depth and stencil testing
	VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo{};
	depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateInfo.depthTestEnable = VK_TRUE;
	depthStencilStateInfo.depthWriteEnable = VK_TRUE;
	depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateInfo.minDepthBounds = 0.0f;
	depthStencilStateInfo.maxDepthBounds = 1.0f;
	depthStencilStateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateInfo.front = {};
	depthStencilStateInfo.back = {};

	// color blending
	// configuration per color attachment
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	// configuration for global color blending settings
	VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{};
	colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendStateInfo.attachmentCount = 1;
	colorBlendStateInfo.pAttachments = &colorBlendAttachment;
	colorBlendStateInfo.blendConstants[0] = 0.0f;
	colorBlendStateInfo.blendConstants[1] = 0.0f;
	colorBlendStateInfo.blendConstants[2] = 0.0f;
	colorBlendStateInfo.blendConstants[3] = 0.0f;

	// dynamic states
	// allows you to specify the data at drawing time
	std::array<VkDynamicState, 2> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateInfo.pDynamicStates = dynamicStates.data();

	// graphics pipeline
	VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
	graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// config all previos objects
	graphicsPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	graphicsPipelineInfo.pStages = shaderStages.data();
	graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	graphicsPipelineInfo.pViewportState = &viewportStateInfo;
	graphicsPipelineInfo.pRasterizationState = &rasterizationStateInfo;
	graphicsPipelineInfo.pMultisampleState = &multisampleStateInfo;
	graphicsPipelineInfo.pDepthStencilState = &depthStencilStateInfo;
	graphicsPipelineInfo.pColorBlendState = &colorBlendStateInfo;
	graphicsPipelineInfo.pDynamicState = &dynamicStateInfo;
	graphicsPipelineInfo.layout = m_PipelineLayout;
	graphicsPipelineInfo.renderPass = m_RenderPass;
	graphicsPipelineInfo.subpass = 0; // index of subpass
	graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineInfo.basePipelineIndex = -1;

	THROW(vkCreateGraphicsPipelines(m_DeviceVk, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &m_Pipeline)
			  != VK_SUCCESS,
		"Failed to create graphics pipeline!");
}

void Engine::CreateCommandBuffers()
{
	m_CommandBuffers.resize(Config::maxFramesInFlight);
	VkCommandBufferAllocateInfo cmdBuffAllocInfo =
		initializers::CommandBufferAllocateInfo(m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()));
	THROW(vkAllocateCommandBuffers(m_DeviceVk, &cmdBuffAllocInfo, m_CommandBuffers.data()) != VK_SUCCESS,
		"Failed to allocate command buffers!")
}

void Engine::CreateSyncObjects()
{
	m_ImageAvailableSemaphores.resize(Config::maxFramesInFlight);
	m_RenderFinishedSemaphores.resize(Config::maxFramesInFlight);
	m_InFlightFences.resize(Config::maxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// create the fence in signaled state so that the first frame doesnt have to wait
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32_t i = 0; i < Config::maxFramesInFlight; ++i)
	{
		THROW(
			vkCreateSemaphore(m_DeviceVk, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS
				|| vkCreateSemaphore(m_DeviceVk, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS
				|| vkCreateFence(m_DeviceVk, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS,
			"Failed to create synchronization objects!")
	}
}


// event callbacks
void Engine::ProcessInput()
{
	// forward input data to ImGui first
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse || io.WantCaptureKeyboard)
		return;

	// if (Input::IsMouseButtonPressed(Mouse::BUTTON_1))
	// {
	// 	// hide cursor when moving camera
	// 	m_Window->HideCursor();
	// }
	// else if (Input::IsMouseButtonReleased(Mouse::BUTTON_1))
	// {
	// 	// unhide cursor when camera stops moving
	// 	m_Window->ShowCursor();
	// }
}

void Engine::OnCloseEvent()
{
	m_IsRunning = false;
}

void Engine::OnResizeEvent(int width, int height)
{
	RecreateSwapchain();
}

void Engine::OnMouseMoveEvent(double xpos, double ypos)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse)
		return;
}

void Engine::OnKeyEvent(int key, int scancode, int action, int mods)
{
	// quits the application
	// works even when the ui is in focus
	if (Input::IsKeyPressed(Key::LEFT_CONTROL) && Input::IsKeyPressed(Key::Q))
		m_IsRunning = false;

	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard)
		return;
}