#include "GameObject.h"

GameObject::~GameObject()
{
    destroy();
}

template <typename T, typename... Args>
std::shared_ptr<T> GameObject::addComponent(Args&&... args)
{
    std::shared_ptr<T> component = std::make_shared<T>(std::forward<Args>(args)...);
    components.push_back(component);

    component->gameObject = shared_from_this();
    component->onCreate();

    return component;
}

template <typename T>
std::shared_ptr<T> GameObject::getComponent()
{
    for (const auto& component : components)
    {
        std::shared_ptr<T> castedComponent = std::dynamic_pointer_cast<T>(component);
        if (castedComponent != nullptr)
        {
            return castedComponent;
        }
    }

    return nullptr;
}

std::shared_ptr<TransformComponent>& GameObject::getTransform()
{
    if (!transform)
    {
        transform = addComponent<TransformComponent>();
    }

    return transform;
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
