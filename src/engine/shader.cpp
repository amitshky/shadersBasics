#include "engine/shader.h"

#include <fstream>
#include "core/core.h"


Shader::Shader(VkDevice deviceVk, const char* path, ShaderType type)
	: m_DeviceVk{ deviceVk },
	  m_Path{ path },
	  m_Type{ type },
	  m_ShaderModule{ VK_NULL_HANDLE },
	  m_ShaderStage{} // has to be default initialized
{
	LoadShader();
	CreateShaderModule();
	CreateShaderStage();
}

Shader::~Shader()
{
	vkDestroyShaderModule(m_DeviceVk, m_ShaderModule, nullptr);
}

void Shader::LoadShader()
{
	std::ifstream file{ m_Path, std::ios::binary | std::ios::ate };
	THROW(!file.is_open(), "Error opening shader file: {}", m_Path)

	const size_t fileSize = static_cast<size_t>(file.tellg());
	m_ShaderCode.resize(fileSize);

	file.seekg(0);
	file.read(m_ShaderCode.data(), static_cast<std::streamsize>(fileSize));
	file.close();
}

void Shader::CreateShaderModule()
{
	VkShaderModuleCreateInfo shaderModuleInfo{};
	shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleInfo.codeSize = m_ShaderCode.size();
	// code is in char but shaderModule expects it to be in uint32_t
	shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(m_ShaderCode.data());

	if (vkCreateShaderModule(m_DeviceVk, &shaderModuleInfo, nullptr, &m_ShaderModule) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shader module!");
}

void Shader::CreateShaderStage()
{
	m_ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	m_ShaderStage.stage = static_cast<VkShaderStageFlagBits>(m_Type);
	m_ShaderStage.module = m_ShaderModule;
	m_ShaderStage.pName = "main";
}