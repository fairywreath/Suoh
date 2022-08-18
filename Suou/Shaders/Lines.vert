#version 460

layout(location = 0) out vec4 lineColor;

layout(binding = 0) uniform UBO
{
	mat4 mvp;
	float time;
} ubo;

struct DrawVertex
{
	float x, y, z;
	float r, g, b, a;
};

layout(binding = 1) readonly buffer SBO
{
	DrawVertex data[];
} sbo;

void main()
{
	DrawVertex v = sbo.data[gl_VertexIndex];

	gl_Position = ubo.mvp * vec4(v.x, v.y, v.z, 1.0);
	lineColor = vec4(v.r, v.g, v.b, v.a);
}

