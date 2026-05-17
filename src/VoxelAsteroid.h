#pragma once
#include <glm/glm.hpp>
#include <random>
#include <cmath>
#include "Types.h"

enum class VoxelType : uint8_t { Empty = 0, Rock = 1, Silver = 2 };

struct VoxelAsteroid {
    static constexpr int   kGrid     = 6;
    static constexpr float kVoxelSize = 1.5f;

    glm::vec3 position;
    glm::vec3 velocity;
    VoxelType grid[kGrid][kGrid][kGrid] = {};
    bool      alive           = true;
    int       voxelsRemaining = 0;

    void generate(std::mt19937& rng) {
        float center = (kGrid - 1) * 0.5f;
        float outerR = kGrid * 0.50f;
        float innerR = kGrid * 0.28f;

        std::uniform_real_distribution<float> jitter(-0.7f, 0.7f);
        std::uniform_real_distribution<float> silver(0.0f,  1.0f);

        voxelsRemaining = 0;
        for (int x = 0; x < kGrid; ++x)
        for (int y = 0; y < kGrid; ++y)
        for (int z = 0; z < kGrid; ++z) {
            float dx = x - center, dy = y - center, dz = z - center;
            float d  = std::sqrt(dx*dx + dy*dy + dz*dz);
            if (d > outerR + jitter(rng)) {
                grid[x][y][z] = VoxelType::Empty;
            } else {
                grid[x][y][z] = (d < innerR && silver(rng) < 0.40f)
                                 ? VoxelType::Silver : VoxelType::Rock;
                ++voxelsRemaining;
            }
        }
    }

    glm::vec3 voxelCenter(int x, int y, int z) const {
        float half = (kGrid - 1) * 0.5f * kVoxelSize;
        return position + glm::vec3(
            x * kVoxelSize - half,
            y * kVoxelSize - half,
            z * kVoxelSize - half
        );
    }

    bool worldToGrid(const glm::vec3& p, int& ox, int& oy, int& oz) const {
        float half = (kGrid - 1) * 0.5f;
        auto rnd = [](float f) {
            return static_cast<int>(std::floor(f + 0.5f));
        };
        ox = rnd((p.x - position.x) / kVoxelSize + half);
        oy = rnd((p.y - position.y) / kVoxelSize + half);
        oz = rnd((p.z - position.z) / kVoxelSize + half);
        return ox >= 0 && ox < kGrid && oy >= 0 && oy < kGrid && oz >= 0 && oz < kGrid;
    }

    AABB bounds() const {
        float r = kGrid * 0.5f * kVoxelSize;
        return { position - glm::vec3(r), position + glm::vec3(r) };
    }
};
