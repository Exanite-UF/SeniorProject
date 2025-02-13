#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

struct Transform
{
    glm::vec3 position = glm::vec3(0, 0, 0);
    glm::quat rotation = glm::quat(1, 0, 0, 0);
    glm::vec3 scale = glm::vec3(1, 1, 1);
};
