#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>

#include <src/gameobjects/Component.h>


class SkyboxComponent : public Component
{
public:
    glm::vec3 sunDirection;
    float sunAngularSize;

    
};
