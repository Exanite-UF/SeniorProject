#include "TransformComponent.h"

#include <src/gameobjects/GameObject.h>
#include <tracy/Tracy.hpp>

bool TransformComponent::tryGetParent(std::shared_ptr<TransformComponent>& outParent) const
{
    assertIsPartOfWorld();

    outParent = this->parent.lock();
    return !!outParent;
}

const std::vector<std::shared_ptr<TransformComponent>>& TransformComponent::getChildren() const
{
    return children;
}

void TransformComponent::updateTransform() const
{
    ZoneScoped;

    auto localTransform = glm::translate(glm::mat4(1.0f), localPosition) * glm::mat4_cast(localRotation) * glm::scale(glm::mat4(1.0f), localScale);

    std::shared_ptr<TransformComponent> parent;
    if (tryGetParent(parent))
    {
        globalTransform = parent->getGlobalTransform() * localTransform;
        globalRotation = parent->getGlobalRotation() * localRotation;
    }
    else
    {
        globalTransform = localTransform;
        globalRotation = localRotation;
    }

    inverseGlobalTransform = glm::inverse(globalTransform);

    for (const auto& child : children)
    {
        child->updateTransform();
    }
}

void TransformComponent::setParent(const std::shared_ptr<TransformComponent>& parent)
{
    ZoneScoped;

    assertIsPartOfWorld();

    // Remove self from parent's child list
    const auto previousParent = this->parent.lock();
    if (previousParent)
    {
        std::erase(previousParent->children, std::dynamic_pointer_cast<TransformComponent>(shared_from_this()));
    }

    // Update parent of self
    this->parent = parent;

    // Update children of new parent
    if (parent)
    {
        parent->children.push_back(std::dynamic_pointer_cast<TransformComponent>(shared_from_this()));
    }
}

bool TransformComponent::hasParent() const
{
    return !this->parent.expired();
}

void TransformComponent::onRemovingFromWorld()
{
    ZoneScoped;

    setParent(nullptr);

    // Destroy all children GameObjects in reverse order
    // Note that this is very different from destroying just the TransformComponent
    auto childrenCopy = children;
    for (int i = childrenCopy.size() - 1; i >= 0; --i)
    {
        auto child = childrenCopy[i];
        if (child->getIsPartOfWorld())
        {
            child->getGameObject()->removeFromWorld();
        }
    }

    children.clear();

    // Destroy own GameObject to ensure the TransformComponent is never destroyed without its GameObject being destroyed
    getGameObject()->removeFromWorld();

    Component::onRemovingFromWorld();
}

const glm::vec3& TransformComponent::getLocalPosition() const
{
    return localPosition;
}

const glm::quat& TransformComponent::getLocalRotation() const
{
    return localRotation;
}

const glm::vec3& TransformComponent::getLocalScale() const
{
    return localScale;
}

glm::vec3 TransformComponent::getRightDirection() const
{
    return getGlobalRotation() * glm::vec3(0, -1, 0);
}

glm::vec3 TransformComponent::getUpDirection() const
{
    return getGlobalRotation() * glm::vec3(0, 0, 1);
}

glm::vec3 TransformComponent::getForwardDirection() const
{
    return getGlobalRotation() * glm::vec3(1, 0, 0);
}

glm::vec3 TransformComponent::getGlobalPosition() const
{
    return glm::vec3(globalTransform[3]);
}

glm::quat TransformComponent::getGlobalRotation() const
{
    return globalRotation;
}

glm::vec3 TransformComponent::getLossyGlobalScale() const
{
    // Decompose matrix: https://math.stackexchange.com/questions/237369/given-this-transformation-matrix-how-do-i-decompose-it-into-translation-rotati
    glm::vec3 x = globalTransform[0];
    glm::vec3 y = globalTransform[1];
    glm::vec3 z = globalTransform[2];

    return glm::vec3(glm::length(x), glm::length(y), glm::length(z));
}

const glm::mat4& TransformComponent::getGlobalTransform() const
{
    return globalTransform;
}

const glm::mat4& TransformComponent::getInverseGlobalTransform() const
{
    return inverseGlobalTransform;
}

void TransformComponent::setLocalPosition(const glm::vec3& value)
{
    localPosition = value;
    updateTransform();
}

void TransformComponent::addLocalPosition(const glm::vec3& value)
{
    localPosition += value;
    updateTransform();
}

void TransformComponent::setLocalRotation(const glm::quat& value)
{
    localRotation = value;
    updateTransform();
}

void TransformComponent::addLocalRotation(const glm::quat& value)
{
    localRotation = value * localRotation;
    updateTransform();
}

void TransformComponent::setLocalScale(const glm::vec3& value)
{
    localScale = value;
    updateTransform();
}

void TransformComponent::multiplyLocalScale(const glm::vec3& value)
{
    localScale *= value;
    updateTransform();
}

void TransformComponent::setGlobalPosition(const glm::vec3& value)
{
    std::shared_ptr<TransformComponent> parent;
    if (tryGetParent(parent))
    {
        setLocalPosition(parent->getGlobalTransform() * glm::vec4(value, 1));
    }
    else
    {
        setLocalPosition(value);
    }
}

void TransformComponent::addGlobalPosition(const glm::vec3& value)
{
    std::shared_ptr<TransformComponent> parent;
    if (tryGetParent(parent))
    {
        setLocalPosition(parent->getGlobalTransform() * glm::vec4(getGlobalPosition() + value, 1));
    }
    else
    {
        addLocalPosition(value);
    }
}

void TransformComponent::setGlobalRotation(const glm::quat& value)
{
    std::shared_ptr<TransformComponent> parent;
    if (tryGetParent(parent))
    {
        setLocalRotation(value * parent->getGlobalRotation());
    }
    else
    {
        setLocalRotation(value);
    }
}

void TransformComponent::addGlobalRotation(const glm::quat& value)
{
    std::shared_ptr<TransformComponent> parent;
    if (tryGetParent(parent))
    {
        setLocalRotation(value * localRotation * parent->getGlobalRotation());
    }
    else
    {
        addLocalRotation(value);
    }
}
