#include "GameObject.h"

GameObject::GameObject()
{
    // Create the transform
    Component* transform = new TransformComponent(this);
    components.push_back(transform);
}

GameObject::~GameObject()
{
    // loop to clear the pointers, seems a bit destructive
    for (Component* component : components)
    {
        delete component;
    }
}

template <typename T, typename... Args>
T* GameObject::addComponent(Args&&... args)
{
    T* component = new T(std::forward<Args>(args)...);
    component->gameObject = this;
    components.push_back(component);
    // component->onCreate();
    return component;
}

template <typename T>
T* GameObject::getComponent()
{
    for (Component* component : components)
    {
        // attempting to cast component to type T
        T* castedComponent = dynamic_cast<T*>(component);
        if (castedComponent != nullptr)
        {
            return castedComponent;
        }
    }

    return nullptr;
}

void GameObject::destroy()
{
}
