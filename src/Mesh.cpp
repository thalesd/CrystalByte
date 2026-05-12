#include "Mesh.h"
#include <tiny_obj_loader.h>
#include <glm/glm.hpp>
#include <stdexcept>
#include <unordered_map>
#include <cmath>
#include <limits>
#include <algorithm>

namespace {
    struct IndexPairHash {
        size_t operator()(std::pair<int, int> const& p) const {
            return std::hash<int>{}(p.first) ^ (std::hash<int>{}(p.second) << 16);
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
    std::unordered_map<std::pair<int, int>, uint32_t, IndexPairHash> indexMap;

    for (const auto& shape : shapes) {
        for (const auto& idx : shape.mesh.indices) {
            auto key = std::make_pair(idx.vertex_index, idx.normal_index);
            auto it  = indexMap.find(key);
            if (it != indexMap.end()) {
                mesh.indices.push_back(it->second);
            } else {
                Vertex v{};
                v.pos.x = attrib.vertices[3 * idx.vertex_index + 0];
                v.pos.y = attrib.vertices[3 * idx.vertex_index + 1];
                v.pos.z = attrib.vertices[3 * idx.vertex_index + 2];

                if (idx.normal_index >= 0) {
                    float nx = attrib.normals[3 * idx.normal_index + 0];
                    float ny = attrib.normals[3 * idx.normal_index + 1];
                    float nz = attrib.normals[3 * idx.normal_index + 2];
                    v.color = glm::vec3(std::abs(nx), std::abs(ny), std::abs(nz));
                } else {
                    v.color = glm::vec3(0.7f, 0.7f, 0.7f);
                }

                uint32_t newIdx  = static_cast<uint32_t>(mesh.vertices.size());
                indexMap[key]    = newIdx;
                mesh.vertices.push_back(v);
                mesh.indices.push_back(newIdx);
            }
        }
    }

    // Normalize to fit within [-1, 1] on the largest axis, centered at origin.
    glm::vec3 minV(std::numeric_limits<float>::max());
    glm::vec3 maxV(std::numeric_limits<float>::lowest());
    for (const auto& v : mesh.vertices) {
        minV = glm::min(minV, v.pos);
        maxV = glm::max(maxV, v.pos);
    }
    glm::vec3 center  = (minV + maxV) * 0.5f;
    float     maxDim  = std::max({ maxV.x - minV.x, maxV.y - minV.y, maxV.z - minV.z });
    float     scale   = (maxDim > 0.0f) ? 2.0f / maxDim : 1.0f;

    for (auto& v : mesh.vertices)
        v.pos = (v.pos - center) * scale;

    return mesh;
}
