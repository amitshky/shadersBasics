#version 450

layout(binding = 0) uniform UniformBufferObject
{
	vec3 iResolution;
	float iTime;
}
ubo;

layout(location = 0) in vec3 inColor;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(inColor, 1.0);
}