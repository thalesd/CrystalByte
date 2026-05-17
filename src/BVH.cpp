#include "BVH.h"
#include "VoxelAsteroid.h"
#include <algorithm>
#include <numeric>
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
// Build — unifies regular asteroids and voxel asteroids into one BVH
// ---------------------------------------------------------------------------

void BVH::build(const std::vector<Asteroid>&      asteroids,
                const std::vector<VoxelAsteroid>& voxelAsteroids) {
    nodes_.clear();

    std::vector<Prim> prims;
    prims.reserve(asteroids.size() + voxelAsteroids.size());

    for (int i = 0; i < (int)asteroids.size(); ++i) {
        if (!asteroids[i].alive) continue;
        prims.push_back({ sphereAABB(asteroids[i].position, asteroids[i].radius), i, false });
    }
    for (int i = 0; i < (int)voxelAsteroids.size(); ++i) {
        if (!voxelAsteroids[i].alive) continue;
        prims.push_back({ voxelAsteroids[i].bounds(), i, true });
    }

    if (prims.empty()) return;

    std::vector<int> ids(prims.size());
    std::iota(ids.begin(), ids.end(), 0);
    nodes_.reserve(prims.size() * 2);
    buildNode(ids, prims, 0, (int)prims.size());
}

int BVH::buildNode(std::vector<int>& ids, const std::vector<Prim>& prims, int start, int end) {
    int idx = (int)nodes_.size();
    nodes_.push_back({});

    AABB box{ glm::vec3(std::numeric_limits<float>::max()),
              glm::vec3(std::numeric_limits<float>::lowest()) };
    for (int i = start; i < end; ++i) {
        box.min = glm::min(box.min, prims[ids[i]].aabb.min);
        box.max = glm::max(box.max, prims[ids[i]].aabb.max);
    }
    nodes_[idx].aabb = box;

    if (end - start == 1) {
        nodes_[idx].objectIndex = prims[ids[start]].index;
        nodes_[idx].isVoxel     = prims[ids[start]].isVoxel;
        return idx;
    }

    glm::vec3 extent = box.max - box.min;
    int axis = (extent.y > extent.x) ? 1 : 0;
    if (extent.z > extent[axis]) axis = 2;

    int mid = (start + end) / 2;
    std::nth_element(ids.begin() + start, ids.begin() + mid, ids.begin() + end,
        [&](int a, int b) {
            glm::vec3 ca = (prims[a].aabb.min + prims[a].aabb.max) * 0.5f;
            glm::vec3 cb = (prims[b].aabb.min + prims[b].aabb.max) * 0.5f;
            return ca[axis] < cb[axis];
        });

    // Access by index after recursion — push_back may reallocate nodes_.
    int L = buildNode(ids, prims, start, mid);
    int R = buildNode(ids, prims, mid,   end);
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

BVHHit BVH::raycast(const glm::vec3& origin, const glm::vec3& dir, float tMax) const {
    BVHHit result;
    result.t = tMax;
    if (nodes_.empty()) return result;

    glm::vec3 invDir = 1.0f / dir;
    float bestT = tMax;

    int stack[64], top = 0;
    stack[top++] = 0;

    while (top > 0) {
        const BVHNode& node = nodes_[stack[--top]];
        if (!hitAABB(origin, invDir, node.aabb, bestT)) continue;

        if (node.left == -1) {
            if (!node.isVoxel) {
                // Regular asteroid: analytic ray-sphere
                glm::vec3 c  = (node.aabb.min + node.aabb.max) * 0.5f;
                float     r  = (node.aabb.max.x - node.aabb.min.x) * 0.5f;
                glm::vec3 oc = origin - c;
                float proj   = glm::dot(oc, dir);
                float disc   = proj * proj - glm::dot(oc, oc) + r * r;
                if (disc < 0.0f) continue;
                float t = -proj - std::sqrt(disc);
                if (t > 0.0f && t < bestT) {
                    bestT          = t;
                    result.index   = node.objectIndex;
                    result.isVoxel = false;
                    result.t       = t;
                }
            } else {
                // Voxel asteroid: AABB entry point
                glm::vec3 t0    = (node.aabb.min - origin) * invDir;
                glm::vec3 t1    = (node.aabb.max - origin) * invDir;
                glm::vec3 tNear = glm::min(t0, t1);
                float tEnter = std::max({ tNear.x, tNear.y, tNear.z, 0.0f });
                if (tEnter > 0.0f && tEnter < bestT) {
                    bestT          = tEnter;
                    result.index   = node.objectIndex;
                    result.isVoxel = true;
                    result.t       = tEnter;
                }
            }
        } else {
            stack[top++] = node.left;
            stack[top++] = node.right;
        }
    }

    return result;
}

void BVH::queryAABB(const AABB& query, std::vector<int>& out) const {
    if (nodes_.empty()) return;

    int stack[64], top = 0;
    stack[top++] = 0;

    while (top > 0) {
        const BVHNode& node = nodes_[stack[--top]];
        if (!node.aabb.intersects(query)) continue;

        if (node.left == -1) {
            if (!node.isVoxel)
                out.push_back(node.objectIndex);
        } else {
            stack[top++] = node.left;
            stack[top++] = node.right;
        }
    }
}
