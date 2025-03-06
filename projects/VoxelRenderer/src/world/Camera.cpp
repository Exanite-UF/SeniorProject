#include <src/world/Camera.h>

float Camera::getHorizontalFov() const
{
    return glm::pi<float>() / 2 * 1.5;
}

glm::vec3 Camera::getRightDirection() const
{
    return glm::vec3(std::sin(rotation.y), -std::cos(rotation.y), 0);
}

glm::vec3 Camera::getUpDirection() const
{
    return glm::vec3(0, 0, 1);
}

glm::vec3 Camera::getForwardDirection() const
{
    return glm::vec3(std::cos(rotation.y), std::sin(rotation.y), 0);
}
