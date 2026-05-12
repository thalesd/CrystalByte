#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>

// Vertex layout sent to the GPU.
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription d{};
        d.binding   = 0;
        d.stride    = sizeof(Vertex);
        d.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return d;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> a{};
        a[0].binding  = 0; a[0].location = 0;
        a[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        a[0].offset   = offsetof(Vertex, pos);
        a[1].binding  = 0; a[1].location = 1;
        a[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        a[1].offset   = offsetof(Vertex, color);
        return a;
    }
};

// Uniform buffer sent to the vertex shader every frame.
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
