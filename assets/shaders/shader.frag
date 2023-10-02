#version 450

layout(binding = 0) uniform UniformBufferObject
{
	vec3 iResolution;
	float iTime;
	mat4 view;
}
ubo;

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

float HitSphere(const vec3 center, const float radius, const Ray r)
{
	// in the quadriatic equation
	// a = dir . dir
	// b = 2 * (dir . (org - center))
	// c = org . org - radius^2
	// replace b = 2h in quadriatic equation
	// (-h +- sqrt(h^2 - ac))/a
	// h = b / 2
	// NOTE: the dot product of itself can be replaced with length squared
	vec3 originToCenter = r.origin - center;
	float a = dot(r.direction, r.direction);
	float h = dot(r.direction, originToCenter);
	float c = dot(originToCenter, originToCenter) - radius * radius;

	float discriminant = h * h - a * c;

	if (discriminant < 0)
		return -1.0;

	// return the value of `t` to calc point of point of hit
	return (-h - sqrt(discriminant)) / a;
}

vec4 RayColor(const Ray r)
{
	vec3 sphereCenter = vec3(0.0, 0.0, -1.0);
	float sphereRadius = 0.5;

	float t = HitSphere(sphereCenter, sphereRadius, r);
	if (t > 0.0)
	{
		// calc normal
		vec3 normal = normalize(RayAt(r, t) - sphereCenter);
		return vec4(0.5 * (normal + vec3(1.0)), 1.0);
	}

	vec3 dir = normalize(r.direction);
	float a = 0.5 * (dir.y + 1.0);
	return vec4((1.0 - a) * vec3(1.0) + a * vec3(0.5, 0.7, 1.0), 1.0);
}


void main()
{
	float aspectRatio = ubo.iResolution.x / ubo.iResolution.y;
	float viewportHeight = 2.0;
	float viewportWidth = viewportHeight * aspectRatio;

	// camera
	float focalLength = 1.0;
	vec3 cameraPos = vec3(0.0, 0.0, 0.0);

	// viewport vectors (horizontal and vertical)
	vec3 viewportU = vec3(viewportWidth, 0.0, 0.0);
	vec3 viewportV = vec3(0.0, -viewportHeight, 0.0);

	// pixel-to-pixel vectors
	vec3 pixelDeltaU = viewportU / ubo.iResolution.x;
	vec3 pixelDeltaV = viewportV / ubo.iResolution.y;

	// upper-left pixel
	vec3 viewportUpperLeft = cameraPos - vec3(0.0, 0.0, focalLength) - viewportU / 2 - viewportV / 2;
	// location of the center pixel of the upper left corner of the viewport
	vec3 pixelLoc = viewportUpperLeft + 0.5 * (pixelDeltaU + pixelDeltaV);

	vec3 pixelCenter = pixelLoc + (gl_FragCoord.x * pixelDeltaU) + (gl_FragCoord.y * pixelDeltaV);
	vec3 rayDir = pixelCenter - cameraPos;

	Ray ray = Ray(cameraPos, rayDir);
	outColor = RayColor(ray);
}