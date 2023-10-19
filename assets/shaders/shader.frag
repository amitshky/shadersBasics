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


const float PI = 3.14159265359;
const float MAX_FLOAT = 1.0 / 0.0;
const float MIN_FLOAT = -1.0 / 0.0;

const int MAX_SAMPLES = 4;
const int MAX_BOUNCES = 8;
const uint NUM_OBJS = 4;


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

/**
 * @param x vec2 to generate random number
 * @returns random float
 */
float hash12(vec2 x)
{
	uint n = baseHash(floatBitsToUint(x));
	return float(n) * (1.0 / float(0xffffffffU));
}

/**
 * @param x vec2 to generate random number
 * @returns random vec2
 */
vec2 hash22(vec2 x)
{
	uint n = baseHash(floatBitsToUint(x));
	uvec2 rz = uvec2(n, n * 48271U);
	return vec2((rz.xy >> 1) & uvec2(0x7fffffffU)) / float(0x7fffffff);
}

/**
 * @param x vec2 to generate random number
 * @returns random vec3
 */
vec3 hash32(vec2 x)
{
	uint n = baseHash(floatBitsToUint(x));
	uvec3 rz = uvec3(n, n * 16807U, n * 48271U);
	return vec3((rz >> 1) & uvec3(0x7fffffffU)) / float(0x7fffffff);
}

/**
 * generates a random values within the range [min, max)
 * @param minVal min value of the range
 * @param maxVal max value of the range
 * @returns random vec3
 */
vec3 randRange3(float minVal, float maxVal)
{
	return minVal + (maxVal - minVal) * hash32(inPosition.xy * ubo.time);
}

/**
 * @param p vec2 to generate random number
 * @returns random vec3 within a unit sphere
 */
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

/**
 * @param x vec2 to generate random number
 * @returns random normalized vec3 within a unit sphere
 */
vec3 randNormSphereVec(vec2 p)
{
	return normalize(randUnitSphere(p));
}

/**
 * @param x vec2 to generate random number
 * @returns random vec3 within a disk (z component = 0.0)
 */
vec3 randUnitDisk(vec2 p)
{
	return vec3(randUnitSphere(p).xy, 0);
}

/**
 * @param normal normal of the surface
 * @returns random vec3 within a unit hemisphere
 */
vec3 randNormHemisphere(const vec3 normal)
{
	vec3 onSphere = randNormSphereVec(normal.xy * ubo.time);
	if (dot(onSphere, normal) > 0.0)
		return onSphere;

	// flip the points if not aligned with the normal
	return -onSphere;
}

// ---------------------------------------

struct Ray
{
	vec3 origin;
	vec3 direction;
};

/**
 * @param r Ray object
 * @param t distance from the ray origin
 * @returns value of the ray hit point at distance `t`
 */
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
	float intercept;
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
	vec3 point; // point of hit
	Primitive obj; // object at point of hit
};

// ---------- hit functions for primitives ----------------
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
	// float a = dot(r.direction, r.direction); // NOTE: if the ray direction is not normalized uncomment this
	float a = 1.0f; // ray direction is normalized so dot product is 1.0
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
	float numerator = plane.intercept - dot(r.origin, plane.normal);
	float denominator = dot(r.direction, plane.normal);

	if (denominator == 0.0)
		return -1.0;

	return numerator / denominator;
}


/**
 * @returns color of the closest object hit
 */
vec4 RayColor(Ray r, inout Primitive objs[NUM_OBJS])
{
	HitRecord rec;
	// WARNING: may not work in every device or version of glsl
	// TODO: find a better way of doing this
	// rec.closestT = MAX_FLOAT; // max float

	float attenuation = 1.0;

	for (int bounces = 0; bounces < MAX_BOUNCES; ++bounces)
	{
		rec.closestT = MAX_FLOAT; // max float
		for (int i = 0; i < NUM_OBJS; ++i)
		{
			if (objs[i].type == SPHERE)
			{
				float t = HitSphere(objs[i].sphere, r);
				if (t <= 0.0)
					continue;

				if (t < rec.closestT)
				{
					rec.closestT = t;
					rec.point = RayAt(r, t);
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
					rec.point = RayAt(r, t);
					rec.obj.type = objs[i].type;
					rec.obj.plane = objs[i].plane;
				}
			}
		}

		if (rec.closestT == MAX_FLOAT)
		{
			// sky
			vec3 dir = r.direction;
			float a = 0.5 * (dir.y + 1.0);
			vec3 skyGradient = (1.0 - a) * vec3(1.0) + a * vec3(0.5, 0.7, 1.0);
			return vec4(attenuation * skyGradient, 1.0);
		}

		if (rec.obj.type == SPHERE)
		{
			rec.normal = (rec.point - rec.obj.sphere.center) / rec.obj.sphere.radius; // normalizing; radius is the magnitude of a vector from the center to the surface of the sphere
			// rec.normal = normalize(rec.point - rec.obj.sphere.center);

			// vec3 lightDir = vec3(0.5, 2.0, 1.0);
			// float intensity = max(dot(lightDir, rec.normal), 0.0) * 0.6;
			// return intensity * vec4(0.4, 0.6, 0.5, 1.0);

			// color = vec4(0.005 * color.xyz, 1.0);
			attenuation *= 0.5;
		}

		if (rec.obj.type == PLANE)
		{
			rec.normal = rec.obj.plane.normal;
			// color = vec4(0.005 * color.xyz, 1.0);
			// return vec4(0.698, 0.698, 0.698, 1.0);
			// return vec4(0.5 * (rec.normal + vec3(1.0)), 1.0);

			attenuation *= 0.5;
		}

		vec3 direction = randNormHemisphere(rec.normal); // this is already normalized
		r = Ray(rec.point, direction);
	}

	return vec4(vec3(0.0), 1.0);
}


void main()
{
	Primitive objs[NUM_OBJS] = Primitive[NUM_OBJS](
		Primitive(SPHERE, Sphere(vec3( 0.0, 0.0, -1.0), 0.5), Plane(vec3(0.0), 0.0)),
		Primitive(SPHERE, Sphere(vec3( 1.2, 0.0, -1.0), 0.5), Plane(vec3(0.0), 0.0)),
		Primitive(SPHERE, Sphere(vec3(-1.2, 0.0, -1.0), 0.5), Plane(vec3(0.0), 0.0)),
		Primitive(SPHERE, Sphere(vec3( 0.0, -500.5001, -1.0), 500.0), Plane(vec3(0.0), 0.0))
		// Primitive(PLANE,  Sphere(vec3(0.0), 0.0), Plane(vec3(0.0, 1.0, 0.0), -0.5001))
	);


	vec4 color = vec4(0.0);
	for (int i = 0; i < MAX_SAMPLES; ++i)
	{
		vec2 p = inPosition.xy * ubo.time;
		vec3 rayDir = normalize(inRayDir + hash32(p));
		vec3 origin = ubo.cameraPos;

		Ray ray = Ray(origin, rayDir);
		color += RayColor(ray, objs);
	}

	// outColor = color / float(MAX_SAMPLES);
	outColor = vec4(sqrt((color / float(MAX_SAMPLES)).xyz), 1.0);


	// vec2 p = inPosition.xy * ubo.time;
	// vec3 rayDir = normalize(inRayDir);
	// vec3 origin = ubo.cameraPos;

	// Ray ray = Ray(origin, rayDir);
	// vec4 color = RayColor(ray, objs);

	// // outColor = vec4((color.xyz), 1.0);
	// outColor = vec4(sqrt(color.xyz), 1.0);
}