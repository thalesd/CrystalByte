#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription d{};
        d.binding   = 0;
        d.stride    = sizeof(Vertex);
        d.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return d;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> a{};
        a[0].binding  = 0; a[0].location = 0;
        a[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        a[0].offset   = offsetof(Vertex, pos);
        a[1].binding  = 0; a[1].location = 1;
        a[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        a[1].offset   = offsetof(Vertex, normal);
        a[2].binding  = 0; a[2].location = 2;
        a[2].format   = VK_FORMAT_R32G32_SFLOAT;
        a[2].offset   = offsetof(Vertex, texCoord);
        return a;
    }
};

struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
};

struct PushConstants {
    glm::mat4 model;
    glm::vec4 baseColor;
};

struct AABB {
    glm::vec3 min, max;

    bool intersects(const AABB& o) const {
        return min.x <= o.max.x && max.x >= o.min.x &&
               min.y <= o.max.y && max.y >= o.min.y &&
               min.z <= o.max.z && max.z >= o.min.z;
    }
};

// Computes the world-space AABB of a local box [localMin, localMax] transformed by m.
// Uses the standard per-axis decomposition — correct for any affine transform.
inline AABB transformAABB(const glm::vec3& localMin, const glm::vec3& localMax, const glm::mat4& m) {
    glm::vec3 t = glm::vec3(m[3]);
    AABB result{ t, t };
    for (int col = 0; col < 3; ++col) {
        for (int row = 0; row < 3; ++row) {
            float e = m[col][row];
            float a = e * localMin[col];
            float b = e * localMax[col];
            if (a < b) { result.min[row] += a; result.max[row] += b; }
            else        { result.min[row] += b; result.max[row] += a; }
        }
    }
    return result;
}
