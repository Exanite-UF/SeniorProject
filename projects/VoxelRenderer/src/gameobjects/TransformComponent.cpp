#include "TransformComponent.h"

#include <src/gameobjects/GameObject.h>

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
