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
	// (-h +- sqrt(h^2 - ac))/a
	// h = b / 2
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
	vec3 pixelPos = inPosition; // normalized device coordinates
	vec4 target = ubo.invProj * vec4(pixelPos, 1.0); // view space
	vec3 rayDir = normalize(vec3(ubo.invView * vec4(normalize(target.xyz / target.w), 0.0))); // world space

	Ray ray = Ray(ubo.cameraPos, rayDir);
	outColor = RayColor(ray);
}