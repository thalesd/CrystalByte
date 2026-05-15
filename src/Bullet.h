#pragma once
#include <glm/glm.hpp>

struct Bullet {
    glm::vec3 position;
    glm::vec3 direction;
    float     speed    = 25.0f;
    float     lifetime =  4.0f;
    bool      alive    = true;
};
