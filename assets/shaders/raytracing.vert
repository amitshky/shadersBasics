#version 450

vec3 positions[6] = vec3[](
	vec3(-1.0, -1.0, 0.0),
	vec3(-1.0,  1.0, 0.0),
	vec3( 1.0,  1.0, 0.0),

	vec3( 1.0,  1.0, 0.0),
	vec3( 1.0, -1.0, 0.0),
	vec3(-1.0, -1.0, 0.0)
);

layout(binding = 0) uniform UniformBufferObject
{
	vec3 resolution;
	float time;
	vec3 cameraPos;
	mat4 model;
	mat4 viewProj;
	mat4 invView;
	mat4 invProj;
	mat4 invViewProj;
}
ubo;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outRayDir;

void main()
{
	outPosition = positions[gl_VertexIndex];
	gl_Position = vec4(outPosition, 1.0);

	// normalized device coords [-1, 1]
	vec2 pixelPos = outPosition.xy;

	vec4 screenSpaceFar = vec4(pixelPos.xy, 1.0, 1.0);
	vec4 screenSpaceNear = vec4(pixelPos.xy, 0.0, 1.0);

	// world space
	vec4 far = ubo.invViewProj * screenSpaceFar;
	far /= far.w;
	vec4 near = ubo.invViewProj * screenSpaceNear;
	near /= near.w;

	outRayDir = far.xyz - near.xyz;
}