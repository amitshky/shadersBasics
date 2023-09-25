#version 450

layout(binding = 0) uniform UniformBufferObject
{
	vec3 iResolution;
	float iTime;
}
ubo;

layout(location = 0) out vec4 outColor;

vec3 palette(float t)
{
	vec3 a = vec3(0.5, 0.5, 0.5);
	vec3 b = vec3(0.5, 0.5, 0.5);
	vec3 c = vec3(1.0, 1.0, 1.0);
	vec3 d = vec3(0.263, 0.416, 0.557);
	return a + b * cos(6.28318 * (c * t + d));
}

void main()
{
	// converting uv to [-1,1] and multiplying the x comp of uv with the aspect ratio
	vec2 uv = (gl_FragCoord.xy * 2.0 - ubo.iResolution.xy) / ubo.iResolution.y;
	// vec2 uv0 = uv;

	vec3 finalCol = vec3(0.0);

	// for (float i = 0.0; i < 3.0; ++i)
	// {
	// 	uv = fract(uv * 1.3) - 0.5;
	// 	float d = length(uv) * exp(-length(uv0));
	// 	vec3 col = palette(length(uv0) + i * 0.5 + ubo.iTime * 0.5);

	// 	d = sin(d * 8.0 + ubo.iTime) / 8.0;
	// 	d = abs(d);
	// 	d = pow(0.01 / d, 1.5); // pow changes the contrast

	// 	finalCol += col * d;
	// }

	float radius = 100.0;

	if ((uv.x * uv.x + uv.y * uv.y) <= (radius / ubo.iResolution.x))
		finalCol = vec3(0.4, 0.4, 0.7);

	outColor = vec4(finalCol, 1.0);
}