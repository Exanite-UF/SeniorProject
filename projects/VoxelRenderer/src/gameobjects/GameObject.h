#pragma once

#include <memory>
#include <vector>

#include <src/gameobjects/TransformComponent.h>
#include <src/utilities/NonCopyable.h>

// Component Types
#include <src/world/Camera.h>

class GameObject : public NonCopyable, public std::enable_shared_from_this<GameObject>
{
private:
    bool isDestroyed = false;

    std::shared_ptr<TransformComponent> transform {};

public:
    GameObject() = default;
    ~GameObject() override;

    std::vector<std::shared_ptr<Component>> components {};

    template <typename T, typename... Args>
    std::shared_ptr<T> addComponent(Args&&... args);

    template <typename T>
    std::shared_ptr<T> getComponent();

    std::shared_ptr<TransformComponent>& getTransform();

    // Calls destroy on all the components in the list
    void destroy();

    bool isAlive() const;
};
