#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "Types.h"
#include "Asteroid.h"

struct VoxelAsteroid;

struct BVHNode {
    AABB aabb;
    int  left        = -1;   // -1 = leaf
    int  right       = -1;
    int  objectIndex = -1;   // leaf: index into asteroids[] or voxelAsteroids[]
    bool isVoxel     = false;
};

struct BVHHit {
    int   index   = -1;    // -1 = no hit
    bool  isVoxel = false;
    float t       = 0.0f;
};

class BVH {
public:
    // Build from regular asteroids and (optionally) voxel asteroids.
    void build(const std::vector<Asteroid>&      asteroids,
               const std::vector<VoxelAsteroid>& voxelAsteroids = {});

    // Nearest hit, either asteroid sphere or voxel asteroid AABB.
    BVHHit raycast(const glm::vec3& origin, const glm::vec3& dir, float tMax) const;

    // AABB overlap — returns only regular-asteroid objectIndex values.
    void queryAABB(const AABB& query, std::vector<int>& out) const;

    bool empty() const { return nodes_.empty(); }
    const std::vector<BVHNode>& nodes() const { return nodes_; }

private:
    struct Prim {
        AABB aabb;
        int  index;
        bool isVoxel;
    };

    std::vector<BVHNode> nodes_;

    int  buildNode(std::vector<int>& ids, const std::vector<Prim>& prims, int start, int end);
    bool hitAABB(const glm::vec3& origin, const glm::vec3& invDir,
                 const AABB& box, float tMax) const;
};
