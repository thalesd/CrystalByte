#include "BVH.h"
#include <algorithm>
#include <cmath>
#include <limits>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static AABB sphereAABB(const glm::vec3& center, float radius) {
    glm::vec3 r(radius);
    return { center - r, center + r };
}

// ---------------------------------------------------------------------------
// Build
// ---------------------------------------------------------------------------

void BVH::build(const std::vector<Asteroid>& asteroids) {
    nodes_.clear();
    std::vector<int> ids;
    ids.reserve(asteroids.size());
    for (int i = 0; i < (int)asteroids.size(); ++i)
        if (asteroids[i].alive)
            ids.push_back(i);
    if (ids.empty()) return;
    nodes_.reserve(ids.size() * 2);
    buildNode(ids, asteroids, 0, (int)ids.size());
}

int BVH::buildNode(std::vector<int>& ids, const std::vector<Asteroid>& asteroids,
                   int start, int end) {
    int idx = (int)nodes_.size();
    nodes_.push_back({});

    // Union AABB of all asteroids in [start, end)
    AABB box{ glm::vec3(std::numeric_limits<float>::max()),
              glm::vec3(std::numeric_limits<float>::lowest()) };
    for (int i = start; i < end; ++i) {
        AABB ab = sphereAABB(asteroids[ids[i]].position, 0.5f);
        box.min = glm::min(box.min, ab.min);
        box.max = glm::max(box.max, ab.max);
    }
    nodes_[idx].aabb = box;

    if (end - start == 1) {
        // Leaf: left/right stay -1
        nodes_[idx].asteroidIndex = ids[start];
        return idx;
    }

    // Split along the longest axis at the spatial median
    glm::vec3 extent = box.max - box.min;
    int axis = (extent.y > extent.x) ? 1 : 0;
    if (extent.z > extent[axis]) axis = 2;

    int mid = (start + end) / 2;
    std::nth_element(ids.begin() + start, ids.begin() + mid, ids.begin() + end,
                     [&](int a, int b) {
                         return asteroids[a].position[axis] < asteroids[b].position[axis];
                     });

    // Access nodes_[idx] by index after recursion — references would be invalid
    // because push_back inside the recursive calls may reallocate the vector.
    int L = buildNode(ids, asteroids, start, mid);
    int R = buildNode(ids, asteroids, mid,   end);
    nodes_[idx].left  = L;
    nodes_[idx].right = R;
    return idx;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool BVH::hitAABB(const glm::vec3& o, const glm::vec3& invDir,
                  const AABB& box, float tMax) const {
    glm::vec3 t0    = (box.min - o) * invDir;
    glm::vec3 t1    = (box.max - o) * invDir;
    glm::vec3 tNear = glm::min(t0, t1);
    glm::vec3 tFar  = glm::max(t0, t1);
    float tEnter = std::max({ tNear.x, tNear.y, tNear.z, 0.0f });
    float tExit  = std::min({ tFar.x,  tFar.y,  tFar.z,  tMax });
    return tEnter <= tExit;
}

int BVH::raycast(const glm::vec3& origin, const glm::vec3& dir,
                 float tMax, float& tOut) const {
    if (nodes_.empty()) return -1;

    glm::vec3 invDir = 1.0f / dir;
    int   best  = -1;
    float bestT = tMax;

    int stack[64], top = 0;
    stack[top++] = 0;

    while (top > 0) {
        const BVHNode& node = nodes_[stack[--top]];
        if (!hitAABB(origin, invDir, node.aabb, bestT)) continue;

        if (node.left == -1) {
            // Leaf: analytic ray-sphere using the AABB center/radius
            glm::vec3 c  = (node.aabb.min + node.aabb.max) * 0.5f;
            float     r  = (node.aabb.max.x - node.aabb.min.x) * 0.5f;
            glm::vec3 oc = origin - c;
            float proj   = glm::dot(oc, dir);
            float disc   = proj * proj - glm::dot(oc, oc) + r * r;
            if (disc < 0.0f) continue;
            float t = -proj - std::sqrt(disc);
            if (t > 0.0f && t < bestT) { bestT = t; best = node.asteroidIndex; }
        } else {
            stack[top++] = node.left;
            stack[top++] = node.right;
        }
    }

    tOut = bestT;
    return best;
}

void BVH::queryAABB(const AABB& query, std::vector<int>& out) const {
    if (nodes_.empty()) return;

    int stack[64], top = 0;
    stack[top++] = 0;

    while (top > 0) {
        const BVHNode& node = nodes_[stack[--top]];
        if (!node.aabb.intersects(query)) continue;

        if (node.left == -1) {
            out.push_back(node.asteroidIndex);
        } else {
            stack[top++] = node.left;
            stack[top++] = node.right;
        }
    }
}
