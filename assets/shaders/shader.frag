#version 450

layout(location = 0) out vec4 outColor;

void main()
{
	// TODO: pass uniforms for the width and height
	const vec3 iResolution = vec3(1280.0, 720.0, 1.0);
	vec2 uv = gl_FragCoord.xy / iResolution.xy;

	outColor = vec4(uv.x, 0.0, 0.0, 1.0);
}