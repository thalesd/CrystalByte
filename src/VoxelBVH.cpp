#include "VoxelAsteroid.h"
#include <algorithm>
#include <numeric>
#include <limits>

// ---------------------------------------------------------------------------
// VoxelBVH — per-asteroid BVH over individual voxels
// ---------------------------------------------------------------------------

void VoxelBVH::rebuild(const VoxelAsteroid& va) {
    nodes_.clear();

    std::vector<Prim> prims;
    prims.reserve(VoxelAsteroid::kGrid * VoxelAsteroid::kGrid * VoxelAsteroid::kGrid);

    for (int x = 0; x < VoxelAsteroid::kGrid; ++x)
    for (int y = 0; y < VoxelAsteroid::kGrid; ++y)
    for (int z = 0; z < VoxelAsteroid::kGrid; ++z) {
        if (va.grid[x][y][z] == VoxelType::Empty) continue;
        prims.push_back({ VoxelAsteroid::packKey(x, y, z), va.voxelAABB(x, y, z) });
    }

    if (prims.empty()) return;

    std::vector<int> ids(prims.size());
    std::iota(ids.begin(), ids.end(), 0);
    nodes_.reserve(prims.size() * 2);
    buildNode(ids, prims, 0, (int)prims.size());
}

int VoxelBVH::buildNode(std::vector<int>& ids, const std::vector<Prim>& prims, int start, int end) {
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
        nodes_[idx].voxelKey = prims[ids[start]].packedKey;
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

    int L = buildNode(ids, prims, start, mid);
    int R = buildNode(ids, prims, mid,   end);
    nodes_[idx].left  = L;
    nodes_[idx].right = R;
    return idx;
}

bool VoxelBVH::hitAABB(const glm::vec3& o, const glm::vec3& invDir, const AABB& box, float tMax) {
    glm::vec3 t0    = (box.min - o) * invDir;
    glm::vec3 t1    = (box.max - o) * invDir;
    glm::vec3 tNear = glm::min(t0, t1);
    glm::vec3 tFar  = glm::max(t0, t1);
    float tEnter = std::max({ tNear.x, tNear.y, tNear.z, 0.0f });
    float tExit  = std::min({ tFar.x,  tFar.y,  tFar.z,  tMax });
    return tEnter <= tExit;
}

int VoxelBVH::queryPoint(const glm::vec3& p) const {
    if (nodes_.empty()) return -1;

    int stack[64], top = 0;
    stack[top++] = 0;

    while (top > 0) {
        const VoxelBVHNode& node = nodes_[stack[--top]];

        if (p.x < node.aabb.min.x || p.x > node.aabb.max.x ||
            p.y < node.aabb.min.y || p.y > node.aabb.max.y ||
            p.z < node.aabb.min.z || p.z > node.aabb.max.z)
            continue;

        if (node.left == -1)
            return node.voxelKey;

        stack[top++] = node.left;
        stack[top++] = node.right;
    }
    return -1;
}

int VoxelBVH::raycast(const glm::vec3& origin, const glm::vec3& dir, float tMax, float& tOut) const {
    if (nodes_.empty()) return -1;

    glm::vec3 invDir = 1.0f / dir;
    int   best  = -1;
    float bestT = tMax;

    int stack[64], top = 0;
    stack[top++] = 0;

    while (top > 0) {
        const VoxelBVHNode& node = nodes_[stack[--top]];
        if (!hitAABB(origin, invDir, node.aabb, bestT)) continue;

        if (node.left == -1) {
            glm::vec3 t0    = (node.aabb.min - origin) * invDir;
            glm::vec3 t1    = (node.aabb.max - origin) * invDir;
            glm::vec3 tNear = glm::min(t0, t1);
            float tEnter = std::max({ tNear.x, tNear.y, tNear.z, 0.0f });
            if (tEnter < bestT) { bestT = tEnter; best = node.voxelKey; }
        } else {
            stack[top++] = node.left;
            stack[top++] = node.right;
        }
    }

    tOut = bestT;
    return best;
}
