#pragma once

#include <memory>
#include <vector>

#include <src/gameobjects/Component.h>
#include <src/utilities/NonCopyable.h>

class TransformComponent;

class GameObject : public NonCopyable, public std::enable_shared_from_this<GameObject>
{
private:
    bool isAlive = true;
    std::string name {};

    std::shared_ptr<TransformComponent> transform {};
    std::vector<std::shared_ptr<Component>> components {};

    static constexpr std::string defaultName = "GameObject";

public:
    GameObject(const std::string& name = defaultName);
    ~GameObject() override;

    static std::shared_ptr<GameObject> createRootObject(const std::string& name = defaultName);
    std::shared_ptr<GameObject> createChildObject(const std::string& name = defaultName);

    template <typename T, typename... Args>
    std::shared_ptr<T> addComponent(Args&&... args);

    template <typename T>
    std::shared_ptr<T> getComponent();

    std::shared_ptr<TransformComponent>& getTransform();

    void destroy();

    bool getIsAlive() const;
    void assertIsAlive() const;

    void setName(const std::string& name);
    const std::string& getName();
};

template <typename T, typename... Args>
std::shared_ptr<T> GameObject::addComponent(Args&&... args)
{
    assertIsAlive();

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
    assertIsAlive();

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
