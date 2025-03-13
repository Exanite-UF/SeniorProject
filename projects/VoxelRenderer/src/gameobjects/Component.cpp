#include "Component.h"

Component::Component() = default;

Component::~Component()
{
    destroy();
}

void Component::destroy()
{
    if (isDestroyed)
    {
        return;
    }

    isDestroyed = true;

    onDestroy();

    gameObject.reset();
    transform.reset();
}

std::shared_ptr<TransformComponent>& Component::getTransform()
{
    return transform;
}

std::shared_ptr<GameObject>& Component::getGameObject()
{
    return gameObject;
}

bool Component::isAlive() const
{
    return !isDestroyed;
}
