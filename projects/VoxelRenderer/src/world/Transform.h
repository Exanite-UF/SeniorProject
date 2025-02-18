#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

class Transform
{
private:
    glm::vec3 position = glm::vec3(0, 0, 0);
    glm::quat rotation = glm::quat(1, 0, 0, 0);
    glm::vec3 scale = glm::vec3(1, 1, 1);

    Transform* parent;

public:
    [[nodiscard]] const glm::vec3& getLocalPosition() const;
    [[nodiscard]] const glm::quat& getLocalRotation() const;
    [[nodiscard]] const glm::vec3& getLocalScale() const;

    [[nodiscard]] glm::vec3 getGlobalPosition() const;
    [[nodiscard]] glm::quat getGlobalRotation() const;
    [[nodiscard]] glm::vec3 getGlobalScale() const;

    void setLocalPosition(const glm::vec3& value);
    void addLocalPosition(const glm::vec3& value);
    void setLocalRotation(const glm::quat& value);
    void addLocalRotation(const glm::quat& value);
    void setLocalScale(const glm::vec3& value);
    void multiplyLocalScale(const glm::vec3& value);

    void setGlobalPosition(const glm::vec3& value);
    void addGlobalPosition(const glm::vec3& value);
    void setGlobalRotation(const glm::quat& value);
    void addGlobalRotation(const glm::quat& value);
    void setGlobalScale(const glm::vec3& value);
    void multiplyGlobalScale(const glm::vec3& value);
};
