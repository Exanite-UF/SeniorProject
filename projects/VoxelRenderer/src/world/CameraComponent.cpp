#include <src/world/CameraComponent.h>

float CameraComponent::getHorizontalFov() const
{
    return glm::pi<float>() / 2;
}

float CameraComponent::getVerticalFov() const
{
    float horizontalFov = getHorizontalFov();
    float aspectRatio = getAspectRatio();
    float verticalFov = 2 * std::atan(std::tan(horizontalFov / 2) / aspectRatio);

    return verticalFov;
}

float CameraComponent::getAspectRatio() const
{
    return resolution.x / resolution.y;
}

float CameraComponent::getNearPlane() const
{
    // TODO: No idea what the renderer actually uses
    return 0.01;
}

float CameraComponent::getFarPlane() const
{
    // TODO: No idea what the renderer actually uses
    return 10000;
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
