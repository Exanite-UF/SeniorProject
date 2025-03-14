#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <src/gameobjects/Component.h>

class TransformComponent : public Component, public std::enable_shared_from_this<TransformComponent>
{
private:
    std::shared_ptr<TransformComponent> parent {};
    std::vector<std::shared_ptr<TransformComponent>> children {};

    glm::vec3 localPosition = glm::vec3(0, 0, 0);
    glm::quat localRotation = glm::quat(1, 0, 0, 0);
    glm::vec3 localScale = glm::vec3(1, 1, 1);

    mutable glm::quat globalRotation = glm::quat(1, 0, 0, 0);
    mutable glm::mat4 globalTransform = glm::mat4(1);
    mutable glm::mat4 inverseGlobalTransform = glm::mat4(1);

    void updateTransform() const;

public:
    [[nodiscard]] const std::shared_ptr<TransformComponent>& getParent() const;
    [[nodiscard]] const std::vector<std::shared_ptr<TransformComponent>>& getChildren() const;

    void onDestroy() override;

    [[nodiscard]] const glm::vec3& getLocalPosition() const;
    [[nodiscard]] const glm::quat& getLocalRotation() const;
    [[nodiscard]] const glm::vec3& getLocalScale() const;

    [[nodiscard]] glm::vec3 getRightDirection() const;
    [[nodiscard]] glm::vec3 getUpDirection() const;
    [[nodiscard]] glm::vec3 getForwardDirection() const;

    [[nodiscard]] glm::vec3 getGlobalPosition() const;
    [[nodiscard]] glm::quat getGlobalRotation() const;
    [[nodiscard]] glm::vec3 getLossyGlobalScale() const;

    [[nodiscard]] const glm::mat4& getGlobalTransform() const;
    [[nodiscard]] const glm::mat4& getInverseGlobalTransform() const;

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

    void addChild(const std::shared_ptr<GameObject>& child);
};
