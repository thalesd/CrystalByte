#include "Camera.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

glm::vec3 Camera::forward() const {
    glm::vec3 f;
    f.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    f.y = std::sin(glm::radians(pitch));
    f.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    return glm::normalize(f);
}

void Camera::processInput(GLFWwindow* window, float deltaTime) {
    glm::vec3 fwd   = forward();
    glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0.0f, 1.0f, 0.0f)));
    float     d     = moveSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W)            == GLFW_PRESS) position += fwd   * d;
    if (glfwGetKey(window, GLFW_KEY_S)            == GLFW_PRESS) position -= fwd   * d;
    if (glfwGetKey(window, GLFW_KEY_A)            == GLFW_PRESS) position -= right * d;
    if (glfwGetKey(window, GLFW_KEY_D)            == GLFW_PRESS) position += right * d;
    if (glfwGetKey(window, GLFW_KEY_SPACE)        == GLFW_PRESS) position.y += d;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) position.y -= d;
}

void Camera::applyMouseDelta(float dx, float dy) {
    yaw   += dx * sensitivity;
    pitch  = std::clamp(pitch + dy * sensitivity, -89.0f, 89.0f);
}
