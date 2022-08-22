#pragma once

#include <limits>

#include "RendererBase.h"

namespace Suoh
{

class CanvasRenderer : public RendererBase
{
public:
    CanvasRenderer(VKRenderDevice* renderDevice, Window* window, const Image& depthImage);
    ~CanvasRenderer() override;

    void recordCommands(VkCommandBuffer commandBuffer) override;

    void clear();

    void drawLine(const vec3& p1, const vec3& p2, const vec4& color);
    void drawPlane(const vec3& orig, const vec3& v1, const vec3& v2, int n1, int n2, float s1, float s2, const vec4& color, const vec4& outlineColor);

    void updateBuffer();
    void updateUniformBuffer(const mat4& mvp, float time);

private:
    struct VertexData
    {
        vec3 position;
        vec4 color;
    };

    struct UniformBuffer
    {
        glm::mat4 mvp;
        float time;
    };

    void createDescriptors();

private:
    std::vector<VertexData> mLines;
    std::vector<Buffer> mStorageBuffers;

    static constexpr auto MAX_LINES_COUNT = std::numeric_limits<unsigned short>::max();
    static constexpr auto MAX_LINES_DATA_SIZE = 2 * MAX_LINES_COUNT * sizeof(VertexData);
};

} // namespace Suoh