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

layout(location = 0) out vec3 outColor;

void main()
{
	vec3 positions[6] = vec3[]( //
		vec3(-0.5, -0.5, 0.0), //
		vec3(-0.5, 0.5, 0.0), //
		vec3(0.5, 0.5, 0.0), //

		vec3(0.5, 0.5, 0.0), //
		vec3(0.5, -0.5, 0.0), //
		vec3(-0.5, -0.5, 0.0) //
	);

	vec3 colors[6] = vec3[]( //
		vec3(1.0, 0.0, 0.0), //
		vec3(0.0, 1.0, 0.0), //
		vec3(0.0, 0.0, 1.0), //

		vec3(0.0, 0.0, 1.0), //
		vec3(0.0, 1.0, 0.0), //
		vec3(1.0, 0.0, 0.0) //
	);

	vec3 pos = positions[gl_VertexIndex];
	outColor = colors[gl_VertexIndex];
	gl_Position = ubo.viewProj * ubo.model * vec4(pos, 1.0);
}