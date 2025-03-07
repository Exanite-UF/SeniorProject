#include "Component.h"

Component::Component()
{
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
