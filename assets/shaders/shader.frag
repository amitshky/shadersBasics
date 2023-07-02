#version 450

layout(binding = 0) uniform UniformBufferObject
{
	vec3 iResolution;
	float iTime;
}
ubo;

layout(location = 0) out vec4 outColor;

void main()
{
	// converting uv to [-1,1] and multiplying the x comp of uv with the aspect ratio
	vec2 uv = (gl_FragCoord.xy * 2.0 - ubo.iResolution.xy) / ubo.iResolution.y;

	float d = length(uv);

	d = sin(d * ubo.iTime) / 8.0 + cos(d * 8.0) / 8.0;
	d = abs(d);
	d = smoothstep(0.0, 0.1, d);

	outColor = vec4(d, d, d, 1.0);
}