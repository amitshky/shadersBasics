#version 450

layout(binding = 0) uniform UniformBufferObject
{
	vec3 iResolution;
	float iTime;
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

vec4 RayColor(const Ray r)
{
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
	vec3 deltaU = viewportU / ubo.iResolution.x;
	vec3 deltaV = viewportV / ubo.iResolution.y;

	// upper-left pixel
	vec3 viewportUpperLeft = cameraPos - vec3(0.0, 0.0, focalLength) - viewportU / 2 - viewportV / 2;
	vec3 pixelLoc = viewportUpperLeft + 0.5 * (deltaU + deltaV);

	vec3 pixelCenter = pixelLoc + (gl_FragCoord.x * deltaU) + (gl_FragCoord.y * deltaV);
	vec3 rayDir = pixelCenter - cameraPos;

	Ray ray = Ray(cameraPos, rayDir);
	outColor = RayColor(ray);
}