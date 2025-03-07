#include "Component.h"

Component::Component(std::shared_ptr<GameObject> parent)
{
    this->gameObject = parent;
}

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

bool Component::isAlive() const
{
    return !isDestroyed;
}
