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
    return 0;
}

template <typename T>
T* GameObject::getComponent()
{
    return 0;
}

void GameObject::destroy()
{

}