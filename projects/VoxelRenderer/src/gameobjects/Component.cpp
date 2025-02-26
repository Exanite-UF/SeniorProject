#include "Component.h"

Component::Component(std::shared_ptr<GameObject> parent)
{
    this->gameObject = parent;
}

Component::~Component()
{
    onDestroy();
    gameObject.reset();
}

void Component::destroy()
{
    // Depends on specific component, will figure out soon
}
