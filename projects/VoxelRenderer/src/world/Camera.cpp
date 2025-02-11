#include <src/world/Camera.h>

glm::quat Camera::getRotation() const
{
    return rotation;
}

glm::vec3 Camera::getPosition() const
{
    return position;
}

float Camera::getHorizontalFov() const
{
    return 3.1415926589 * 0.5;
}
