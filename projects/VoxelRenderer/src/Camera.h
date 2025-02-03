#pragma once
#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera{
public:
    glm::vec3 position;
    glm::quat orientation;

    glm::quat getOrientation() const;
    glm::vec3 getPosition() const;
    float getHorizontalFov() const;
};