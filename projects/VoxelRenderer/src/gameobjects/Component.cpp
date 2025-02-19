#include "Component.h"

Component::Component(GameObject* _parent)
{
    gameObject = _parent;
}

Component::~Component()
{
    // nothing here yet
}

void Component::destroy()
{
    //
}