#pragma once
#include <glm/glm.hpp>

struct PickupItem {
    enum class Type { Metal };
    glm::vec3 position;
    glm::vec3 velocity;
    Type      type     = Type::Metal;
    float     lifetime = 25.0f;
    bool      alive    = true;
};
