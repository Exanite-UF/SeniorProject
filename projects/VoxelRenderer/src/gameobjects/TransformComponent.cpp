#include "TransformComponent.h"

#include <src/gameobjects/GameObject.h>

void TransformComponent::onDestroy()
{
    Component::onDestroy();

    // Destroy all children GameObjects
    // Note that this is very different from destroying just the TransformComponent
    for (int i = 0; i < children.size(); ++i)
    {
        children[i]->gameObject->destroy();
    }
}

const glm::vec3& TransformComponent::getLocalPosition() const
{
    return position;
}

const glm::quat& TransformComponent::getLocalRotation() const
{
    return rotation;
}

const glm::vec3& TransformComponent::getLocalScale() const
{
    return scale;
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

// TODO: This currently does the same as the local version. Needs a proper implementation.
glm::vec3 TransformComponent::getGlobalPosition() const
{
    return position;
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
glm::quat TransformComponent::getGlobalRotation() const
{
    return rotation;
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
glm::vec3 TransformComponent::getGlobalScale() const
{
    return scale;
}

void TransformComponent::setLocalPosition(const glm::vec3& value)
{
    position = value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

void TransformComponent::addLocalPosition(const glm::vec3& value)
{
    position += value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

void TransformComponent::setLocalRotation(const glm::quat& value)
{
    rotation = value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

void TransformComponent::addLocalRotation(const glm::quat& value)
{
    rotation = value * rotation;
    // TODO: Propagate transform changes to children or mark self as dirty
}

void TransformComponent::setLocalScale(const glm::vec3& value)
{
    scale = value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

void TransformComponent::multiplyLocalScale(const glm::vec3& value)
{
    scale *= value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
void TransformComponent::setGlobalPosition(const glm::vec3& value)
{
    position = value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
void TransformComponent::addGlobalPosition(const glm::vec3& value)
{
    position += value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
void TransformComponent::setGlobalRotation(const glm::quat& value)
{
    rotation = value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
void TransformComponent::addGlobalRotation(const glm::quat& value)
{
    rotation = value * rotation;
    // TODO: Propagate transform changes to children or mark self as dirty
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
void TransformComponent::setGlobalScale(const glm::vec3& value)
{
    scale = value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
void TransformComponent::multiplyGlobalScale(const glm::vec3& value)
{
    scale *= value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

void TransformComponent::addChild(std::shared_ptr<GameObject> child)
{
    children.push_back(child->getTransform());
    
    child->getTransform()->parent = shared_from_this();
}
