#include "Mesh.h"
#include <tiny_obj_loader.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <stdexcept>
#include <unordered_map>
#include <cmath>
#include <limits>
#include <algorithm>
#include <tuple>

namespace {
    using IndexKey = std::tuple<int, int, int>; // vertex, normal, texcoord

    struct IndexKeyHash {
        size_t operator()(const IndexKey& k) const {
            auto hc = [](size_t seed, size_t v) {
                return seed ^ (v + 0x9e3779b9u + (seed << 6) + (seed >> 2));
            };
            size_t h = std::hash<int>{}(std::get<0>(k));
            h = hc(h, std::hash<int>{}(std::get<1>(k)));
            h = hc(h, std::hash<int>{}(std::get<2>(k)));
            return h;
        }
    };
}

Mesh Mesh::loadOBJ(const std::string& path) {
    tinyobj::ObjReaderConfig cfg;
    cfg.triangulate = true;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path, cfg))
        throw std::runtime_error("tinyobjloader: " + reader.Error());

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();

    Mesh mesh;
    std::unordered_map<IndexKey, uint32_t, IndexKeyHash> indexMap;

    for (const auto& shape : shapes) {
        for (const auto& idx : shape.mesh.indices) {
            auto key = std::make_tuple(idx.vertex_index, idx.normal_index, idx.texcoord_index);
            auto it  = indexMap.find(key);
            if (it != indexMap.end()) {
                mesh.indices.push_back(it->second);
            } else {
                Vertex v{};

                v.pos.x = attrib.vertices[3 * idx.vertex_index + 0];
                v.pos.y = attrib.vertices[3 * idx.vertex_index + 1];
                v.pos.z = attrib.vertices[3 * idx.vertex_index + 2];

                if (idx.normal_index >= 0) {
                    v.normal.x = attrib.normals[3 * idx.normal_index + 0];
                    v.normal.y = attrib.normals[3 * idx.normal_index + 1];
                    v.normal.z = attrib.normals[3 * idx.normal_index + 2];
                } else {
                    v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

                if (idx.texcoord_index >= 0) {
                    v.texCoord.x =        attrib.texcoords[2 * idx.texcoord_index + 0];
                    v.texCoord.y = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]; // flip V for Vulkan
                } else {
                    v.texCoord = glm::vec2(0.0f, 0.0f);
                }

                uint32_t newIdx = static_cast<uint32_t>(mesh.vertices.size());
                indexMap[key]   = newIdx;
                mesh.vertices.push_back(v);
                mesh.indices.push_back(newIdx);
            }
        }
    }

    // Normalize positions to [-1, 1] on the largest axis, centered at origin.
    glm::vec3 minV(std::numeric_limits<float>::max());
    glm::vec3 maxV(std::numeric_limits<float>::lowest());
    for (const auto& v : mesh.vertices) {
        minV = glm::min(minV, v.pos);
        maxV = glm::max(maxV, v.pos);
    }
    glm::vec3 center = (minV + maxV) * 0.5f;
    float     maxDim = std::max({ maxV.x - minV.x, maxV.y - minV.y, maxV.z - minV.z });
    float     scale  = (maxDim > 0.0f) ? 2.0f / maxDim : 1.0f;

    for (auto& v : mesh.vertices)
        v.pos = (v.pos - center) * scale;
    // Normals are direction vectors — uniform scale doesn't change their direction

    return mesh;
}

Mesh Mesh::makeSphere(int stacks, int sectors) {
    Mesh mesh;
    const float pi = glm::pi<float>();

    for (int i = 0; i <= stacks; ++i) {
        float phi = pi * static_cast<float>(i) / static_cast<float>(stacks);
        for (int j = 0; j <= sectors; ++j) {
            float theta = 2.0f * pi * static_cast<float>(j) / static_cast<float>(sectors);

            Vertex v{};
            v.pos.x    = std::sin(phi) * std::cos(theta);
            v.pos.y    = std::cos(phi);
            v.pos.z    = std::sin(phi) * std::sin(theta);
            v.normal   = v.pos; // unit sphere: normal == position
            v.texCoord = { static_cast<float>(j) / sectors,
                           static_cast<float>(i) / stacks };
            mesh.vertices.push_back(v);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            uint32_t a = static_cast<uint32_t>(i * (sectors + 1) + j);
            uint32_t b = a + static_cast<uint32_t>(sectors + 1);
            mesh.indices.push_back(a);
            mesh.indices.push_back(b);
            mesh.indices.push_back(a + 1);
            mesh.indices.push_back(b);
            mesh.indices.push_back(b + 1);
            mesh.indices.push_back(a + 1);
        }
    }

    return mesh;
}
