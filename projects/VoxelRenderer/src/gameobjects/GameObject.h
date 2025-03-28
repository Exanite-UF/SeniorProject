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
private:
    // GameObjects aren't supposed to be thread safe, but this provides a bit of extra safety
    std::atomic<bool> isAlive = true;
    std::atomic<bool> wasPublicDestroyCalled = false;
    std::atomic<bool> wasDestroyNotified = false;
    std::atomic<bool> isDestroyComplete = false;

    std::string name {};

    std::shared_ptr<TransformComponent> transform {};
    std::vector<std::shared_ptr<Component>> components {};

    static constexpr const char* defaultName = "GameObject";

    void notifyDestroy();

    void actualDestroy();

public:
    explicit GameObject(const std::string& name = defaultName);
    ~GameObject() override;

    void update();
    void destroy();

    static std::shared_ptr<GameObject> createRootObject(const std::string& name = defaultName);
    std::shared_ptr<GameObject> createChildObject(const std::string& name = defaultName);

    template <typename T, typename... Args>
    std::shared_ptr<T> addComponent(Args&&... args);

    template <typename T>
    std::shared_ptr<T> getComponent();

    std::shared_ptr<TransformComponent>& getTransform();

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
    component->notifyCreate();

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
