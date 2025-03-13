#pragma once

#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <src/utilities/NonCopyable.h>

#include <src/gameobjects/TransformComponent.h> // maybe replace with component

class CameraComponent : public Component
{
public:
    // For Input
    glm::vec2 rotation = glm::vec2(0);

    float moveSpeed = 32;
    float mouseSensitivity = 0.002;

    [[nodiscard]] float getHorizontalFov() const;

    [[nodiscard]] glm::vec3 getRightMoveDirection() const;
    [[nodiscard]] glm::vec3 getForwardMoveDirection() const;
    [[nodiscard]] glm::vec3 getUpMoveDirection() const;
};
