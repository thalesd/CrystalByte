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
