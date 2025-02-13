#include "Transform.h"

glm::vec3 Transform::getPosition() const
{
    return position;
}

glm::quat Transform::getRotation() const
{
    return rotation;
}

glm::vec3 Transform::getScale() const
{
    return scale;
}

void Transform::setLocalPosition(const glm::vec3& value)
{
    position = value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

void Transform::addLocalPosition(const glm::vec3& value)
{
    position += value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

void Transform::setLocalRotation(const glm::quat& value)
{
    rotation = value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

void Transform::addLocalRotation(const glm::quat& value)
{
    rotation = value * rotation;
    // TODO: Propagate transform changes to children or mark self as dirty
}

void Transform::setLocalScale(const glm::vec3& value)
{
    scale = value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

void Transform::multiplyLocalScale(const glm::vec3& value)
{
    scale *= value;
    // TODO: Propagate transform changes to children or mark self as dirty
}
