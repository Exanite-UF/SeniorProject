#include "Component.h"

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
}

std::shared_ptr<TransformComponent>& Component::getTransform()
{
    return gameObject->getTransform();
}

std::shared_ptr<GameObject>& Component::getGameObject()
{
    return gameObject;
}

bool Component::isAlive() const
{
    return !isDestroyed;
}
