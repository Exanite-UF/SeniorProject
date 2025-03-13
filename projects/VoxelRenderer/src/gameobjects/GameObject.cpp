#include "GameObject.h"

#include "TransformComponent.h"

GameObject::GameObject() = default;

GameObject::~GameObject()
{
    destroy();
}

std::shared_ptr<TransformComponent>& GameObject::getTransform()
{
    return transform;
}

std::shared_ptr<GameObject> GameObject::create()
{
    auto gameObject = std::shared_ptr<GameObject>(new GameObject());

    // Add default Transform component
    auto transform = gameObject->addComponent<TransformComponent>();

    // Set transform
    // This is required because even though addComponent initializes transform->transform,
    // it doesn't set it to the correct value due to a circular dependency.
    // Doing this avoids that circular dependency.
    transform->transform = transform;

    return gameObject;
}

void GameObject::destroy()
{
    if (isDestroyed)
    {
        return;
    }

    isDestroyed = true;

    for (const auto& component : components)
    {
        component->destroy();
    }

    components.clear();
}

bool GameObject::isAlive() const
{
    return !isDestroyed;
}
