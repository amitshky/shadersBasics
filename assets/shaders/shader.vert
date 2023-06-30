#version 450

vec3 positions[6] = vec3[]( //
	vec3(-1.0, -1.0, 0.0), //
	vec3(-1.0, 1.0, 0.0), //
	vec3(1.0, 1.0, 0.0), //

	vec3(1.0, 1.0, 0.0), //
	vec3(1.0, -1.0, 0.0), //
	vec3(-1.0, -1.0, 0.0) //
);

void main()
{
	gl_Position = vec4(positions[gl_VertexIndex], 1.0);
}