#version 450

vec3 positions[6] = vec3[]( //
	vec3(-1.0, -1.0, 0.0), //
	vec3(-1.0, 1.0, 0.0), //
	vec3(1.0, 1.0, 0.0), //

	vec3(1.0, 1.0, 0.0), //
	vec3(1.0, -1.0, 0.0), //
	vec3(-1.0, -1.0, 0.0) //
);


layout(location = 0) out vec3 outPosition;

void main()
{
	outPosition = positions[gl_VertexIndex];
	gl_Position = vec4(outPosition, 1.0);
}