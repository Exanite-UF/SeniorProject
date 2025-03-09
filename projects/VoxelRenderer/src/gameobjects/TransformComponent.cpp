#include "TransformComponent.h"

#include <src/gameobjects/GameObject.h>

const std::shared_ptr<TransformComponent>& TransformComponent::getParent() const
{
    return parent;
}

const std::vector<std::shared_ptr<TransformComponent>>& TransformComponent::getChildren() const
{
    return children;
}

void TransformComponent::updateTransform() const
{
    if (!isDirty)
    {
        return;
    }

    localTransform = glm::translate(glm::mat4(1.0f), position) *
    glm::mat4_cast(rotation) *
    glm::scale(glm::mat4(1.0f), scale);

    if (parent)
    {
        globalTransform = parent->getGlobalTransform() * localTransform;
    }
    else
    {
        globalTransform = localTransform;
    }

    isDirty = false;
}

void TransformComponent::markDirty()
{
    isDirty = true;

    for (auto& child : children)
    {
        child->markDirty();
    }
}


void TransformComponent::onDestroy()
{
    Component::onDestroy();

    // Destroy all children GameObjects
    // Note that this is very different from destroying just the TransformComponent
    for (int i = 0; i < children.size(); ++i)
    {
        children[i]->getGameObject()->destroy();
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

glm::vec3 TransformComponent::getGlobalPosition() const
{
    // Recursively obtains global position
    return (parent != nullptr) ? (position + parent->getGlobalPosition()) : position;
}

glm::quat TransformComponent::getGlobalRotation() const
{
    //Rotation order might be incorrect need to check
    return (parent != nullptr) ? (parent->getGlobalRotation() * rotation) : rotation;
}

glm::vec3 TransformComponent::getGlobalScale() const
{
    return (parent != nullptr) ? (scale * parent->getGlobalScale()) : scale;
}

const glm::mat4& TransformComponent::getLocalTransform() const
{
    updateTransform();
    return localTransform;
}

const glm::mat4& TransformComponent::getGlobalTransform() const
{
    updateTransform();
    return globalTransform;
}



void TransformComponent::setLocalPosition(const glm::vec3& value)
{
    position = value;
    markDirty();
}

void TransformComponent::addLocalPosition(const glm::vec3& value)
{
    position += value;
    markDirty();
}

void TransformComponent::setLocalRotation(const glm::quat& value)
{
    rotation = value;
    markDirty();
}

void TransformComponent::addLocalRotation(const glm::quat& value)
{
    rotation = value * rotation;
    markDirty();
}

void TransformComponent::setLocalScale(const glm::vec3& value)
{
    scale = value;
    markDirty();
}

void TransformComponent::multiplyLocalScale(const glm::vec3& value)
{
    scale *= value;
    markDirty();
}

void TransformComponent::setGlobalPosition(const glm::vec3& value)
{
    (parent != nullptr) ? (position = value + parent->getGlobalPosition()) : (position = value);
    markDirty();
}

void TransformComponent::addGlobalPosition(const glm::vec3& value)
{
    (parent != nullptr) ? (position += value + parent->getGlobalPosition()) : (position += value);
    markDirty();
}

void TransformComponent::setGlobalRotation(const glm::quat& value)
{
    (parent != nullptr) ? (rotation = value * parent->getGlobalRotation()) : (rotation = value);
    markDirty();
}

void TransformComponent::addGlobalRotation(const glm::quat& value)
{
    (parent != nullptr) ? (rotation = value * parent->getGlobalRotation() * rotation) : (rotation = value * rotation);
    markDirty();
}

void TransformComponent::setGlobalScale(const glm::vec3& value)
{
    (parent != nullptr) ? (scale = value * parent->getGlobalScale()) : (scale = value);
    markDirty();
}

void TransformComponent::multiplyGlobalScale(const glm::vec3& value)
{
    (parent != nullptr) ? (scale *= value * parent->getGlobalScale()) : (scale *= value);
    markDirty();
}

void TransformComponent::addChild(std::shared_ptr<GameObject> child)
{
    children.push_back(child->getTransform());

    child->getTransform()->parent = shared_from_this();
}
