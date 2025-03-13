#pragma once

#include <memory>
#include <vector>

#include <src/gameobjects/Component.h>
#include <src/utilities/NonCopyable.h>

class TransformComponent;

class GameObject : public NonCopyable, public std::enable_shared_from_this<GameObject>
{
    friend class std::shared_ptr<GameObject>;

private:
    bool isDestroyed = false;

    std::shared_ptr<TransformComponent> transform {};
    std::vector<std::shared_ptr<Component>> components {};

    GameObject();

public:
    ~GameObject() override;

    static std::shared_ptr<GameObject> create();

    template <typename T, typename... Args>
    std::shared_ptr<T> addComponent(Args&&... args);

    template <typename T>
    std::shared_ptr<T> getComponent();

    std::shared_ptr<TransformComponent>& getTransform();

    void destroy();

    bool isAlive() const;
};

template <typename T, typename... Args>
std::shared_ptr<T> GameObject::addComponent(Args&&... args)
{
    std::shared_ptr<T> component = std::make_shared<T>(std::forward<Args>(args)...);
    components.push_back(component);

    // Initialize the component's gameObject and transform fields
    component->gameObject = shared_from_this();
    component->transform = transform;

    // Notify component of creation
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
