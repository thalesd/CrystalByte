#pragma once
#include <glm/glm.hpp>
#include <random>
#include <cmath>
#include <vector>
#include "Types.h"

enum class VoxelType : uint8_t { Empty = 0, Rock = 1, Silver = 2 };

// -----------------------------------------------------------------------
// Forward declaration — VoxelBVH uses VoxelAsteroid, VoxelAsteroid holds VoxelBVH
// -----------------------------------------------------------------------
struct VoxelAsteroid;

// -----------------------------------------------------------------------
// Per-asteroid BVH over individual voxels
// Implementations live in VoxelBVH.cpp where VoxelAsteroid is a complete type.
// -----------------------------------------------------------------------

struct VoxelBVHNode {
    AABB aabb;
    int  left     = -1;   // -1 = leaf
    int  right    = -1;
    int  voxelKey = -1;   // leaf: gx | (gy<<4) | (gz<<8)
};

class VoxelBVH {
public:
    // Rebuild from the current alive voxels of one asteroid.
    void rebuild(const VoxelAsteroid& va);

    // Returns packed voxel key of the voxel AABB that contains p, or -1.
    int queryPoint(const glm::vec3& p) const;

    // Returns packed key of nearest voxel hit by ray, -1 on miss. Sets tOut.
    int raycast(const glm::vec3& origin, const glm::vec3& dir, float tMax, float& tOut) const;

    bool empty() const { return nodes_.empty(); }

private:
    struct Prim { int packedKey; AABB aabb; };
    std::vector<VoxelBVHNode> nodes_;

    int  buildNode(std::vector<int>& ids, const std::vector<Prim>& prims, int start, int end);
    static bool hitAABB(const glm::vec3& o, const glm::vec3& invDir, const AABB& box, float tMax);
};

// -----------------------------------------------------------------------
// VoxelAsteroid
// -----------------------------------------------------------------------

struct VoxelAsteroid {
    static constexpr int   kGrid     = 6;
    static constexpr float kVoxelSize = 1.5f;

    glm::vec3 position;
    glm::vec3 velocity;
    VoxelType grid[kGrid][kGrid][kGrid] = {};
    bool      alive           = true;
    int       voxelsRemaining = 0;
    VoxelBVH  voxelBVH;

    static int  packKey(int gx, int gy, int gz) { return gx | (gy << 4) | (gz << 8); }
    static void unpackKey(int key, int& gx, int& gy, int& gz) {
        gx = key & 0xF; gy = (key >> 4) & 0xF; gz = (key >> 8) & 0xF;
    }

    void generate(std::mt19937& rng) {
        float center = (kGrid - 1) * 0.5f;
        float outerR = kGrid * 0.50f;
        float innerR = kGrid * 0.28f;

        std::uniform_real_distribution<float> jitter(-0.7f, 0.7f);
        std::uniform_real_distribution<float> silver(0.0f, 1.0f);

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
        voxelBVH.rebuild(*this);
    }

    glm::vec3 voxelCenter(int x, int y, int z) const {
        float half = (kGrid - 1) * 0.5f * kVoxelSize;
        return position + glm::vec3(
            x * kVoxelSize - half,
            y * kVoxelSize - half,
            z * kVoxelSize - half
        );
    }

    AABB voxelAABB(int x, int y, int z) const {
        glm::vec3 c = voxelCenter(x, y, z);
        float h = kVoxelSize * 0.5f;
        return { c - glm::vec3(h), c + glm::vec3(h) };
    }

    bool worldToGrid(const glm::vec3& p, int& ox, int& oy, int& oz) const {
        float half = (kGrid - 1) * 0.5f;
        auto rnd = [](float f) { return static_cast<int>(std::floor(f + 0.5f)); };
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
