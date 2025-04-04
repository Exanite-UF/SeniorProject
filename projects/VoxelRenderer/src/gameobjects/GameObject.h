#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <src/gameobjects/Component.h>
#include <src/utilities/CountInstances.h>
#include <src/utilities/NonCopyable.h>

class TransformComponent;

class GameObject : public NonCopyable, public CountInstances<GameObject>, public std::enable_shared_from_this<GameObject>
{
    friend class Component;

private:
    static constexpr const char* defaultName = "GameObject";
    std::string name {};

    // GameObjects aren't supposed to be thread safe, but atomic provides a bit of extra safety
    std::atomic<bool> isPartOfWorld = true;
    std::atomic<bool> wasRemovingFromWorldNotified = false;
    std::atomic<bool> wasRemoveFromWorldCalled = false;

    std::shared_ptr<TransformComponent> transform {};
    std::vector<std::shared_ptr<Component>> components {};

    static constexpr const char* defaultName = "GameObject";

    void notifyDestroy();

    void actualDestroy();

public:
    explicit GameObject(const std::string& name = defaultName);

    void notifyRemovingFromWorld();
    void actualRemoveFromWorld();

public:
    ~GameObject() override;

    static std::shared_ptr<GameObject> createRootObject(const std::string& name = defaultName);
    std::shared_ptr<GameObject> createChildObject(const std::string& name = defaultName);

    void update();
    void removeFromWorld();

    template <typename T, typename... Args>
    std::shared_ptr<T> addComponent(Args&&... args);

    template <typename T>
    std::shared_ptr<T> getComponent();
    std::shared_ptr<TransformComponent>& getTransform();

    bool getIsWorldRoot() const;
    bool getIsPartOfWorld() const;
    void assertIsPartOfWorld() const;

    void setName(const std::string& name);
    const std::string& getName();
};

template <typename T, typename... Args>
std::shared_ptr<T> GameObject::addComponent(Args&&... args)
{
    assertIsPartOfWorld();

    std::shared_ptr<T> component = std::make_shared<T>(std::forward<Args>(args)...);
    components.push_back(component);

    // Initialize the component's gameObject and transform fields
    component->gameObject = shared_from_this();
    component->transform = transform;

    // Notify component
    component->notifyAddedToWorld();

    return component;
}

template <typename T>
std::shared_ptr<T> GameObject::getComponent()
{
    assertIsPartOfWorld();

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
