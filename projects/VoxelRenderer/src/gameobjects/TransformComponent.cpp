#include "TransformComponent.h"

TransformComponent::TransformComponent(std::shared_ptr<GameObject> gameObject)
    : Component(gameObject)
{
    // used preexisting transform class
    transform = new Transform();
}

TransformComponent::~TransformComponent()
{
    delete transform;
    onDestroy();
    gameObject.reset();
}

void TransformComponent::destroy()
{
    // not sure if transform needs a destory, might make it virtual
}
