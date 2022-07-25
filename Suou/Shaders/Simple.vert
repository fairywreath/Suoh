#version 460

// layout(location = 0) out vec3 fragColor;
layout(location = 2) out vec2 uv;

struct VertexData {
    float x;
    float y;
    float z;

    float u;
    float v;
};

// uniform - 1 per shader program pass
layout(binding = 0) uniform UniformBuffer {
    mat4 mvp;
} ubo;


// readonly buffer - only 1 buffer per shader program
layout(binding = 1) readonly buffer Vertices
{
    VertexData data[];
} in_Vertices;

layout(binding = 2) readonly buffer Indices
{
    uint data[];
} in_Indices;


void main()
{
    // use gl_VertexIndex to as index of SSBO
    uint idx = in_Indices.data[gl_VertexIndex];

    VertexData vertex = in_Vertices.data[idx];
    vec3 pos = vec3(vertex.x, vertex.y, vertex.z);

    gl_Position = ubo.mvp * vec4(pos, 1.0);

    // fragColor = pos;
    uv = vec2(vertex.u, vertex.v);
}