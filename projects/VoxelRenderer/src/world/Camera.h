#pragma once

#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <src/utilities/NonCopyable.h>

#include <src/gameobjects/Component.h>

class Camera : public Component, NonCopyable
{
public:
    // TODO: Migrate the camera transforms into it's gameobject's transforms
    glm::vec2 rotation = glm::vec2(0);

    float moveSpeed = 32;
    float mouseSensitivity = 0.002;

    [[nodiscard]] float getHorizontalFov() const;

    [[nodiscard]] glm::vec3 getRightDirection() const;
    [[nodiscard]] glm::vec3 getForwardDirection() const;
    [[nodiscard]] glm::vec3 getUpDirection() const;
};
