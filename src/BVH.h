#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "Types.h"
#include "Asteroid.h"

struct BVHNode {
    AABB aabb;
    int  left          = -1; // -1 = leaf
    int  right         = -1;
    int  asteroidIndex = -1; // filled at leaves only
};

class BVH {
public:
    // Rebuild from the alive subset of asteroids.
    void build(const std::vector<Asteroid>& asteroids);

    // Nearest ray-sphere hit along [origin, origin + dir*tMax).
    // Returns the asteroid index, or -1 if nothing is hit. Sets tOut to hit distance.
    int  raycast(const glm::vec3& origin, const glm::vec3& dir,
                 float tMax, float& tOut) const;

    // Appends to out the index of every asteroid whose AABB overlaps query.
    void queryAABB(const AABB& query, std::vector<int>& out) const;

    bool empty() const { return nodes_.empty(); }
    const std::vector<BVHNode>& nodes() const { return nodes_; }

private:
    std::vector<BVHNode> nodes_;

    int  buildNode(std::vector<int>& ids, const std::vector<Asteroid>& asteroids,
                   int start, int end);
    bool hitAABB(const glm::vec3& origin, const glm::vec3& invDir,
                 const AABB& box, float tMax) const;
};
