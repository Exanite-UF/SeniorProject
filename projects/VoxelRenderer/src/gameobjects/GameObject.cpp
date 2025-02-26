#include "GameObject.h"

GameObject::GameObject()
{
    // Create the transform 
    auto transform = std::make_shared<TransformComponent>(shared_from_this());
    //this->addComponent(std::weak_ptr<Component>(transform)); <- this isnt working and ik it's something to do with the weak pointer
    components.push_back(std::weak_ptr<Component>(transform)); // temporary
}

GameObject::~GameObject()
{
    this->destroy();
    components.clear();
}

template <typename T, typename... Args>
std::shared_ptr<T> GameObject::addComponent(Args&&... args)
{
    std::shared_ptr<T> component = std::make_shared<T>(std::forward<Args>(args)...);
    component->gameObject = shared_from_this();
    components.push_back(std::weak_ptr<Component>(component));
    component->onCreate();
    return component;
}

template <typename T>
std::shared_ptr<T> GameObject::getComponent()
{
    for (const std::weak_ptr<Component>& weakComponent : components)
    {
        // attempting to cast component to type T
        if (auto component = weakComponent.lock())
        {
            std::shared_ptr<T> castedComponent = std::dynamic_pointer_cast<T>(component);
        }
        if (castedComponent != nullptr)
        {
            return castedComponent;
        }
    }

    return nullptr;
}

void GameObject::destroy()
{
    for (const std::weak_ptr<Component>& weakComponent : components)
    {
        if (auto component = weakComponent.lock())
        {
            component->destroy();
        }
    }
}
