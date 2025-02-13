#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

struct Transform
{
    // TODO: Not sure if we want these to be public or private
    glm::vec3 position = glm::vec3(0, 0, 0);
    glm::quat rotation = glm::quat(1, 0, 0, 0); // TODO: Isn't the identity quaternion (0, 0, 0, 1). GLM has a built in function that returns this exact value
    // glm::quat rotation = glm::identity<glm::quat>();
    glm::vec3 scale = glm::vec3(1, 1, 1);

    [[nodiscard]] glm::vec3 getPosition() const;
    [[nodiscard]] glm::quat getRotation() const;
    [[nodiscard]] glm::vec3 getScale() const;

    void setLocalPosition(const glm::vec3& value);
    void addLocalPosition(const glm::vec3& value);
    void setLocalRotation(const glm::quat& value);
    void addLocalRotation(const glm::quat& value);
    void setLocalScale(const glm::vec3& value);
    void multiplyLocalScale(const glm::vec3& value);
};
