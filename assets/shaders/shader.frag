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


#define ENABLE_SAMPLING 0

const float PI = 3.14159265359;
const float MAX_FLOAT = 1.0 / 0.0;
const float MIN_HIT_BIAS = 0.001; // prevents shadow acne caused by lack of floating point precision

const uint NUM_OBJS = 4;
const uint MAX_SAMPLES = 4;
const uint MAX_BOUNCES = 1 << 6; // 2^n


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
 * @param `x` vec2 to generate random number
 * @returns random float
 */
float hash12(vec2 x)
{
	uint n = baseHash(floatBitsToUint(x));
	return float(n) * (1.0 / float(0xffffffffU));
}

/**
 * @param `x` vec2 to generate random number
 * @returns random vec2
 */
vec2 hash22(vec2 x)
{
	uint n = baseHash(floatBitsToUint(x));
	uvec2 rz = uvec2(n, n * 48271U);
	return vec2((rz.xy >> 1) & uvec2(0x7fffffffU)) / float(0x7fffffff);
}

/**
 * @param `x` vec2 to generate random number
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
 * @param `minVal` min value of the range
 * @param `maxVal` max value of the range
 * @returns random vec3
 */
vec3 randRange3(float minVal, float maxVal)
{
	return minVal + (maxVal - minVal) * hash32(inPosition.xy * ubo.time);
}

/**
 * @param `p` vec2 to generate random number
 * @returns random vec3 within a unit sphere
 */
vec3 randUnitSphere(vec2 p)
{
	vec3 rand = hash32(p * ubo.time);
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
 * @param `x` vec2 to generate random number
 * @returns normalized random vec3 within a unit sphere
 */
vec3 randNormSphereVec(vec2 p)
{
	return normalize(randUnitSphere(p));
}

/**
 * @param `x` vec2 to generate random number
 * @returns random vec3 within a disk (z component = 0.0)
 */
vec3 randUnitDisk(vec2 p)
{
	return vec3(randUnitSphere(p).xy, 0);
}

/**
 * @param `normal` normal of the surface
 * @returns normalized random vec3 within a unit hemisphere
 */
vec3 randNormHemisphere(const vec3 normal)
{
	vec3 onSphere = randNormSphereVec(normal.xy);
	if (dot(onSphere, normal) > 0.0)
		return onSphere;

	// flip the points if not aligned with the normal
	return -onSphere;
}

// ---------------------------------------

// NOTE: Always normalize the direction when initializing
struct Ray
{
	vec3 origin;
	vec3 direction;
};

/**
 * @param `r` Ray object
 * @param `t` distance from the ray origin
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

// types of materials
const uint LAMBERTIAN = 0; // diffuse
const uint METAL = 1;

struct Sphere
{
	vec3 center;
	float radius;
};

struct Plane
{
	vec3 normal;
	vec3 position; // a point on the plane
};

// like a base class for prmitives
// only one primitive will be used
// you can check that using the type
struct Primitive
{
	uint type;
	Sphere sphere;
	Plane plane;
	uint material;
};

// to record the closest object hit
struct HitRecord
{
	float closestT;
	vec3 normal;
	vec3 point; // point of hit
	Primitive obj; // object at point of hit
	uint material; // type of material
};

// ---------- hit functions for primitives ----------------
/**
 * @param `sphere`
 * @param `r` ray
 * @param `rec` used to pass the hit info
 * calculates if the ray hit the sphere and stores the hit info in `rec` if it did
 */
bool HitSphere(const Sphere sphere, const Ray r, inout HitRecord rec)
{
	// equation of a sphere
	// (x - c) . (x - c) - r ^ 2 = 0
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
		return false;

	float t = (-h - sqrt(discriminant)) / a;
	if (t > MIN_HIT_BIAS && t < rec.closestT)
	{
		rec.closestT = t;
		rec.point = RayAt(r, t);
		rec.obj.type = SPHERE;
		rec.obj.sphere = sphere;
		// radius is the magnitude of a vector from the center to the surface of the sphere
		// so we are basically normalizing the normal vector of the sphere
		rec.normal = (rec.point - rec.obj.sphere.center) / rec.obj.sphere.radius;
		return true; // no need to calc further if the camera is outside the sphere
	}

	// useful if the camera is inside the sphere
	t = (-h + sqrt(discriminant)) / a;
	if (t > MIN_HIT_BIAS && t < rec.closestT)
	{
		rec.closestT = t;
		rec.point = RayAt(r, t);
		rec.obj.type = SPHERE;
		rec.obj.sphere = sphere;
		// radius is the magnitude of a vector from the center to the surface of the sphere
		// so we are basically normalizing the normal vector of the sphere
		rec.normal = (rec.point - rec.obj.sphere.center) / rec.obj.sphere.radius;
		return true;
	}

	return false;
}

/**
 * @param `plane`
 * @param `r` ray
 * @param `rec` used to pass the hit info
 * calculates if the ray hit the sphere and stores the hit info in `rec` if it did
 */
bool HitPlane(const Plane plane, const Ray r, inout HitRecord rec)
{
	// equation of a plane
	// (p - p0) . n = 0
	// p is any point on the plane
	// p0 is position (of a point on the plane)
	// n is normal of the plane
	float numerator = dot(plane.position - r.origin, plane.normal);
	float denominator = dot(r.direction, plane.normal);

	// the ray did not intersect the plane
	if (denominator == 0.0)
		return false;

	float t = numerator / denominator;
	if (t > MIN_HIT_BIAS && t < rec.closestT)
	{
		rec.closestT = t;
		rec.point = RayAt(r, t);
		rec.obj.type = PLANE;
		rec.obj.plane = plane;
		rec.normal = rec.obj.plane.normal;

		return true;
	}

	return false;
}

/**
 * calls the appropriate hit function and stores the hit info in `rec`
 * the hit info initially has the furthest value of `t` (the
 * scalar parameter of the parametric equation of a line (ray)),
 * and if the ray instersects an object `t` is updated
 *
 * @param `objs` list of objects (primitives)
 * @param `r` ray
 * @param `rec` used to pass the hit info
 * @returns true if the ray intersects the objects, and false if it doesn't.
 */
bool Hit(inout Primitive objs[NUM_OBJS], const Ray r, inout HitRecord rec)
{
	bool isHit = false;
	rec.closestT = MAX_FLOAT;

	for (int i = 0; i < NUM_OBJS; ++i)
	{
		switch (objs[i].type)
		{
		case SPHERE:
			if (HitSphere(objs[i].sphere, r, rec))
			{
				isHit = true;
				rec.material = objs[i].material;
			}
			break;
		case PLANE:
			if (HitPlane(objs[i].plane, r, rec))
			{
				isHit = true;
				rec.material = objs[i].material;
			}
			break;
		}
	}

	return isHit;
}


vec3 LambertianScatter(const vec3 normal)
{
	// instead of just randomly casting rays, the rays should be scattered towards the direction of the normal
	return normal + randNormHemisphere(normal);
}

vec3 MetalScatter(const vec3 rayDir, const vec3 normal)
{
	return rayDir - 2 * dot(rayDir, normal) * normal;
}


/**
 * @param `r` ray
 * @param `objs` list of objects (primitives)
 * @returns color of the closest object hit
 */
vec4 TraceRay(Ray r, inout Primitive objs[NUM_OBJS])
{
	const vec3 albedo = vec3(0.45, 0.6, 0.3);
	vec3 attenuation = vec3(1.0);

	HitRecord rec;
	for (uint bounces = 0; bounces < MAX_BOUNCES; ++bounces)
	{
		// if hit, then attenuate the color and
		// cast the ray in a random direction
		if (Hit(objs, r, rec))
		{
			attenuation *= albedo;
			vec3 direction = vec3(0.0);

			switch (rec.material)
			{
			case LAMBERTIAN:
				direction = normalize(LambertianScatter(rec.normal));
				break;
			case METAL:
				direction = normalize(MetalScatter(r.direction, rec.normal));
				break;
			}

			r = Ray(rec.point, direction);
			continue;
		}

		// if the ray doesn't intesect anything while bouncing,
		// return the attenuated ambient color
		vec3 dir = r.direction;
		float a = 0.5 * (dir.y + 1.0);
		vec3 skyGradient = (1.0 - a) * vec3(1.0) + a * vec3(0.5, 0.7, 1.0);
		return vec4(attenuation * skyGradient, 1.0);
	}

	return vec4(0.0, 0.0, 0.0, 1.0);
}


void main()
{
	Primitive objs[NUM_OBJS] = Primitive[NUM_OBJS](
		Primitive(SPHERE, Sphere(vec3( 1.2, 0.0, -1.0), 0.5), Plane(vec3(0.0), vec3(0.0)), LAMBERTIAN), // right sphere
		Primitive(SPHERE, Sphere(vec3( 0.0, 0.0, -1.0), 0.5), Plane(vec3(0.0), vec3(0.0)), METAL), // middle sphere
		Primitive(SPHERE, Sphere(vec3(-1.2, 0.0, -1.0), 0.5), Plane(vec3(0.0), vec3(0.0)), LAMBERTIAN), // left sphere
		Primitive(SPHERE, Sphere(vec3( 0.0, -500.501, -1.0), 500.0), Plane(vec3(0.0), vec3(0.0)), LAMBERTIAN) // ground sphere
		// Primitive(PLANE,  Sphere(vec3(0.0), 0.0), Plane(vec3(0.0, 1.0, 0.0), vec3(0.0, -0.501, 0.0)), LAMBERTIAN) // ground plane
	);

#if ENABLE_SAMPLING
	vec4 color = vec4(0.0);
	for (uint i = 0; i < MAX_SAMPLES; ++i)
	{
		vec2 p = inPosition.xy * ubo.time;
		vec3 rayDir = normalize(inRayDir + hash32(p));
		vec3 origin = ubo.cameraPos;

		Ray ray = Ray(origin, rayDir);
		color += TraceRay(ray, objs);
	}

	// outColor = color / float(MAX_SAMPLES);
	outColor = vec4(sqrt((color / float(MAX_SAMPLES)).xyz), 1.0);
#else
	vec2 p = inPosition.xy * ubo.time;
	vec3 rayDir = normalize(inRayDir);
	vec3 origin = ubo.cameraPos;

	Ray ray = Ray(origin, rayDir);
	vec4 color = TraceRay(ray, objs);

	// outColor = vec4((color.xyz), 1.0);
	outColor = vec4(sqrt(color.xyz), 1.0);
#endif
}