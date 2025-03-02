#pragma once

#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <src/utilities/NonCopyable.h>

#include <src/world/Transform.h>

class Camera : NonCopyable
{
public:
    Transform transform;
    glm::vec2 rotation = glm::vec2(0);

    float moveSpeed = 32;
    float mouseSensitivity = 0.002;

    [[nodiscard]] float getHorizontalFov() const;

    glm::vec3 getRightDirection() const;
    glm::vec3 getForwardDirection() const;
    glm::vec3 getUpDirection() const;
};
