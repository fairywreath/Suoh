#version 460

layout(location = 0) in vec3 uvw;

layout(location = 0) out vec4 outColor;

/*
 * Material.
 */
layout(binding = 5) readonly buffer MatBO
{
	uint data[];
} matNo;

void main()
{
	outColor = vec4(uvw, 0.5);
	// outColor = vec4(vec3(1.0, 0, 0), 1.0);

}