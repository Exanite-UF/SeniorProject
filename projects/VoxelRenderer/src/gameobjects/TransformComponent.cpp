#include "TransformComponent.h"

TransformComponent::TransformComponent(std::shared_ptr<GameObject> gameObject)
    : Component(gameObject)
{
}

void TransformComponent::onDestroy()
{
    Component::onDestroy();

    // Destroy all children GameObjects
    // Note that this is very different from destroying just the TransformComponent
    for (int i = 0; i < children.size(); ++i)
    {
        children[i]->gameObject->destroy();
    }
}
