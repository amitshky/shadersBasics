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
	mat4 invViewProj;
}
ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inRayDir;

layout(location = 0) out vec4 outColor;

const int MAX_SAMPLES = 10;


// ---------------------------------------

// random number generator
// https://www.shadertoy.com/view/Xt3cDn
uint baseHash(uvec2 p)
{
	p = 1103515245U * ((p >> 1U) ^ (p.yx));
	uint h32 = 1103515245U * ((p.x) ^ (p.y >> 3U));
	return h32 ^ (h32 >> 16);
}

float hash12(vec2 x)
{
	uint n = baseHash(floatBitsToUint(x));
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

// ---------------------------------------

struct Ray
{
	vec3 origin;
	vec3 direction;
};

vec3 RayAt(const Ray r, float t)
{
	return r.origin + t * r.direction;
}

struct Sphere
{
	vec3 center;
	float radius;
};

float HitSphere(const Sphere sphere, const Ray r)
{
	// in the quadriatic equation
	// a = dir . dir
	// b = 2 * (dir . (org - center))
	// c = org . org - radius^2
	// replace b = 2h in quadriatic equation
	// (-h +- sqrt(h^2 - ac)) / a
	// NOTE: the dot product of itself can be replaced with length squared
	vec3 originToCenter = r.origin - sphere.center;
	float a = dot(r.direction, r.direction);
	float h = dot(r.direction, originToCenter);
	float c = dot(originToCenter, originToCenter) - sphere.radius * sphere.radius;

	float discriminant = h * h - a * c;

	if (discriminant < 0)
		return -1.0;

	// return the value of `t` to calc point of point of hit
	return (-h - sqrt(discriminant)) / a;
}

vec4 RayColor(const Ray r)
{
	Sphere sphere = Sphere(vec3(0.0, 0.0, -1.0), 0.5);

	float t = HitSphere(sphere, r);
	if (t > 0.0)
	{
		// calc normal
		vec3 normal = normalize(RayAt(r, t) - sphere.center);
		return vec4(0.5 * (normal + vec3(1.0)), 1.0);
	}

	vec3 dir = normalize(r.direction);
	float a = 0.5 * (dir.y + 1.0);
	return vec4((1.0 - a) * vec3(1.0) + a * vec3(0.5, 0.7, 1.0), 1.0);
}

void main()
{
	vec4 color = vec4(0.0);

	for (int i = 0; i < MAX_SAMPLES; ++i)
	{
		vec3 rayDir = normalize(inRayDir + hash32(inPosition.xy * ubo.time));
		Ray ray = Ray(ubo.cameraPos, rayDir);
		color += RayColor(ray);
	}

	outColor = color / float(MAX_SAMPLES);
}