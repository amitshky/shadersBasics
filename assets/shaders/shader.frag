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

const int MAX_SAMPLES = 32;
const float PI = 3.14159265359;
const float MAX_FLOAT = 1.0 / 0.0;
const float MIN_FLOAT = -1.0 / 0.0;


// ---------------------------------------

// random number generator
// https://www.shadertoy.com/view/Xt3cDn
// https://www.shadertoy.com/view/tl23Rm
// https://www.shadertoy.com/view/7tBXDh
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

vec3 randRange3(float minVal, float maxVal)
{
	return minVal + (maxVal - minVal) * hash32(inPosition.xy * ubo.time);
}

vec3 randUnitSphere(vec2 p)
{
	vec3 rand = hash32(p);
	float phi = 2.0 * PI * rand.x;
	float cosTheta = 2.0 * rand.y - 1.0;
	float u = rand.z;

	float theta = acos(cosTheta);
	float r = pow(u, 1.0 / 3.0);

	float x = r * sin(theta) * cos(phi);
	float y = r * sin(theta) * sin(phi);
	float z = r * cos(theta);

	return vec3(x, y, z);
}

vec3 randUnitVector(vec2 p)
{
	return normalize(randUnitSphere(p));
}

vec3 randUnitDisk(vec2 p)
{
	return vec3(randUnitSphere(p).xy, 0);
}

vec3 randHemisphere(const vec3 normal)
{
	vec3 onSphere = randUnitVector(normal.xy);
	if (dot(onSphere, normal) > 0.0)
		return onSphere;

	return -onSphere;
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

// --------- primitive objects ------------------

// types of primitives
const uint SPHERE = 0;
const uint PLANE = 1;

struct Sphere
{
	vec3 center;
	float radius;
};

struct Plane
{
	vec3 normal;
	float yIntercept;
};

// like a base class for prmitives
// only one primitive will be used
// you can check that using the type
struct Primitive
{
	uint type;
	Sphere sphere;
	Plane plane;
};

// to record the closest object hit
struct HitRecord
{
	float closestT;
	vec3 normal;
	Primitive obj;
};

// ---------- hit functions fro primitives ----------------
/**
 * @returns -1 if there is no hit, else returns the value of `t`
 * which is the hit distance from the ray's origin
 */
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

/**
 * @returns -1 if there is no hit, else returns the value of `t`
 * which is the hit distance from the ray's origin
 */
float HitPlane(const Plane plane, const Ray r)
{
	float numerator = plane.yIntercept - dot(r.origin, plane.normal);
	float denominator = dot(r.direction, plane.normal);

	if (denominator == 0.0)
		return -1.0;

	return numerator / denominator;
}

vec4 RayColor(const Ray r)
{
	Primitive objs[4] = Primitive[](
		Primitive(SPHERE, Sphere(vec3( 0.0, 0.0, -1.0), 0.5), Plane(vec3(0.0), 0.0)),
		Primitive(SPHERE, Sphere(vec3( 1.2, 0.0, -1.0), 0.5), Plane(vec3(0.0), 0.0)),
		Primitive(SPHERE, Sphere(vec3(-1.2, 0.0, -1.0), 0.5), Plane(vec3(0.0), 0.0)),
		Primitive(PLANE,  Sphere(vec3(0.0), 0.0), Plane(vec3(0.0, 1.0, 0.0), -0.5001))
	);

	HitRecord rec;
	// WARNING: may not work in every device of version of glsl
	// TODO: find a better way of doing this
	rec.closestT = MAX_FLOAT; // max float

	for (int i = 0; i < 4; ++i)
	{
		if (objs[i].type == SPHERE)
		{
			float t = HitSphere(objs[i].sphere, r);
			if (t <= 0.0)
				continue;

			if (t < rec.closestT)
			{
				rec.closestT = t;
				rec.obj.type = objs[i].type;
				rec.obj.sphere = objs[i].sphere;
			}
		}
		else if (objs[i].type == PLANE)
		{
			float t = HitPlane(objs[i].plane, r);
			if (t <= 0.0)
				continue;

			if (t < rec.closestT)
			{
				rec.closestT = t;
				rec.obj.type = objs[i].type;
				rec.obj.plane = objs[i].plane;
			}
		}
	}

	if (rec.closestT == MAX_FLOAT)
	{
		// sky
		vec3 dir = normalize(r.direction);
		float a = 0.5 * (dir.y + 1.0);
		return vec4((1.0 - a) * vec3(1.0) + a * vec3(0.5, 0.7, 1.0), 1.0);
	}

	if (rec.obj.type == SPHERE)
	{
		vec3 normal = normalize(RayAt(r, rec.closestT) - rec.obj.sphere.center);
		return vec4(0.5 * (normal + vec3(1.0)), 1.0);
	}

	if (rec.obj.type == PLANE)
	{
		vec3 normal = rec.obj.plane.normal;
		return vec4(0.5 * (normal + vec3(1.0)), 1.0);
	}

	// prolly won't reach here, but just in case
	return vec4(1.0, 0.0, 1.0, 1.0); // magenta
}

void main()
{
	vec4 color = vec4(0.0);

	for (int i = 0; i < MAX_SAMPLES; ++i)
	{
		vec2 p = inPosition.xy * ubo.time;
		vec3 rayDir = normalize(inRayDir + hash32(p));
		vec3 origin = ubo.cameraPos;

		Ray ray = Ray(origin, rayDir);
		color += RayColor(ray);
	}

	outColor = color / float(MAX_SAMPLES);
}