#include <src/world/CameraComponent.h>

float CameraComponent::getHorizontalFov() const
{
    return glm::pi<float>() / 2;
}

glm::vec3 CameraComponent::getRightMoveDirection() const
{
    return glm::vec3(std::sin(rotation.y), -std::cos(rotation.y), 0);
}

glm::vec3 CameraComponent::getUpMoveDirection() const
{
    return glm::vec3(0, 0, 1);
}

glm::vec3 CameraComponent::getForwardMoveDirection() const
{
    return glm::vec3(std::cos(rotation.y), std::sin(rotation.y), 0);
}
