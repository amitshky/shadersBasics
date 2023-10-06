#version 450

layout(binding = 0) uniform UniformBufferObject
{
	vec3 resolution;
	float time;
	vec3 cameraPos;
	mat4 model;
	mat4 viewProj;
	mat4 invView;
	mat4 invProj;
}
ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec4 outColor;

// https://thebookofshaders.com/10/
float rand(vec2 coords)
{
	// coords *= ubo.time;
	return fract(sin(dot(coords.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main()
{
	outColor = vec4(vec3(rand(inPosition.xy)), 1.0);
}