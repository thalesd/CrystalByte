#pragma once
#include <glm/glm.hpp>

struct GLFWwindow;

class Camera {
public:
    glm::vec3 position    = glm::vec3(0.0f, 1.5f, 4.0f);
    float     yaw         = -90.0f;
    float     pitch       = -15.0f;
    float     moveSpeed   =   3.0f;
    float     sensitivity =   0.12f;

    glm::vec3 forward() const;
    void      processInput(GLFWwindow* window, float deltaTime);
    void      applyMouseDelta(float dx, float dy);
};
