#include <src/world/Camera.h>

glm::quat Camera::getRotation() const
{
    return transform.rotation;
}

glm::vec3 Camera::getPosition() const
{
    return transform.position;
}

float Camera::getHorizontalFov() const
{
    return 3.1415926589 * 0.5;
}
