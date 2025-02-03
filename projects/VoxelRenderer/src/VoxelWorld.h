#pragma once

#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/glm.hpp>

class VoxelWorld{
public:
    void bindVoxelTextures() const;
    void unbindVoxelTexture() const;

    glm::quat getOrientation() const;
    glm::vec3 getPosition() const;
    glm::vec3 getScale() const;
};