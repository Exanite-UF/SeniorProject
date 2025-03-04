#pragma once

#include <src/gameobjects/Component.h>
#include <src/gameobjects/TransformComponent.h>
#include <src/utilities/NonCopyable.h>
#include <vector>

class GameObject : public NonCopyable, public std::enable_shared_from_this<GameObject>
{
public:
    GameObject();
    ~GameObject();

    std::vector<std::weak_ptr<Component>> components;

    template <typename T, typename... Args>
    std::shared_ptr<T> addComponent(Args&&... args);

    template <typename T>
    std::shared_ptr<T> getComponent();

    // Calls destroy on all the components in the list
    void destroy();

    // components.at(0) should be the built-in TransformComponent || constructor

    // template <class T>
    // getComponent<T>
    // addComponent<T>

    // gameObject.destroy()
    // component.destroy()
    // destroy() should respect the transform hierarchy
};
