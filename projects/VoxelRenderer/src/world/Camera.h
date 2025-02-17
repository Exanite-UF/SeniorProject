#pragma once

#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <src/world/Transform.h>

class Camera
{
public:
    Transform transform;

    float getHorizontalFov() const;

    // TODO: Do we still want to use these?
    static glm::vec3 getForward(float theta, float phi)
    {
        return { std::cos(theta), std::sin(theta), 0 };
    }

    static glm::vec3 getRight(float theta, float phi)
    {
        return { std::sin(theta), -std::cos(theta), 0 };
    }
};
