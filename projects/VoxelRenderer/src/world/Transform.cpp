#include "Transform.h"

const glm::vec3& Transform::getLocalPosition() const
{
    return position;
}

const glm::quat& Transform::getLocalRotation() const
{
    return rotation;
}

const glm::vec3& Transform::getLocalScale() const
{
    return scale;
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
glm::vec3 Transform::getGlobalPosition() const
{
    return position;
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
glm::quat Transform::getGlobalRotation() const
{
    return rotation;
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
glm::vec3 Transform::getGlobalScale() const
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

// TODO: This currently does the same as the local version. Needs a proper implementation.
void Transform::setGlobalPosition(const glm::vec3& value)
{
    position = value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
void Transform::addGlobalPosition(const glm::vec3& value)
{
    position += value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
void Transform::setGlobalRotation(const glm::quat& value)
{
    rotation = value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
void Transform::addGlobalRotation(const glm::quat& value)
{
    rotation = value * rotation;
    // TODO: Propagate transform changes to children or mark self as dirty
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
void Transform::setGlobalScale(const glm::vec3& value)
{
    scale = value;
    // TODO: Propagate transform changes to children or mark self as dirty
}

// TODO: This currently does the same as the local version. Needs a proper implementation.
void Transform::multiplyGlobalScale(const glm::vec3& value)
{
    scale *= value;
    // TODO: Propagate transform changes to children or mark self as dirty
}
