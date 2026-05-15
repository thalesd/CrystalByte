#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Asteroid {
    glm::vec3 position;
    glm::vec3 rotAxis;
    float     rotAngle = 0.0f;
    float     rotSpeed = 30.0f; // degrees per second
    bool      alive    = true;

    glm::mat4 modelMatrix() const {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), position);
        m = glm::rotate(m, glm::radians(rotAngle), rotAxis);
        m = glm::scale(m, glm::vec3(0.5f)); // 50% of the ship's 2-unit normalized extent
        return m;
    }
};
