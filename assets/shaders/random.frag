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
	coords *= ubo.time;
	return fract(sin(dot(coords.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

//--------------------------------------------------

// https://www.shadertoy.com/view/Xt3cDn
// https://www.shadertoy.com/view/tl23Rm
uint baseHash(uvec2 p)
{
	p = 1103515245U * ((p >> 1U) ^ (p.yx));
	uint h32 = 1103515245U * ((p.x) ^ (p.y >> 3U));
	return h32 ^ (h32 >> 16);
}

float hash12(uvec2 x)
{
	uint n = baseHash(x);
	return float(n) * (1.0 / float(0xffffffffU));
}

vec2 hash22(vec2 x)
{
	uint n = baseHash(floatBitsToUint(x));
	uvec2 rz = uvec2(n, n * 48271U);
	return vec2((rz.xy >> 1) & uvec2(0x7fffffffU)) / float(0x7fffffff);
}

vec3 hash32(vec2 x)
{
	uint n = baseHash(floatBitsToUint(x));
	uvec3 rz = uvec3(n, n * 16807U, n * 48271U);
	return vec3((rz >> 1) & uvec3(0x7fffffffU)) / float(0x7fffffff);
}

//--------------------------------------------------

void main()
{
	// outColor = vec4(vec3(rand(inPosition.xy * ubo.time)), 1.0);
	outColor = vec4(vec3(hash32(inPosition.xy * ubo.time)), 1.0);
}