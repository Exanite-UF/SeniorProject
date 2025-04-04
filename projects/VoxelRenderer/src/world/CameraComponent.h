#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>

#include <src/gameobjects/TransformComponent.h> // maybe replace with component

class CameraComponent : public Component
{
public:
    // For Input
    glm::vec2 rotation {};

    float moveSpeed = 32;
    float mouseSensitivity = 0.002;

    glm::vec2 resolution {};

    [[nodiscard]] float getHorizontalFov() const;
    [[nodiscard]] float getVerticalFov() const;
    [[nodiscard]] float getAspectRatio() const;
    [[nodiscard]] float getNearPlane() const;
    [[nodiscard]] float getFarPlane() const;

    [[nodiscard]] glm::vec3 getRightMoveDirection() const;
    [[nodiscard]] glm::vec3 getForwardMoveDirection() const;
    [[nodiscard]] glm::vec3 getUpMoveDirection() const;
};
