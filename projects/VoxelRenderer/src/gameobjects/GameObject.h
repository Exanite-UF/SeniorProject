#pragma once

#include <src/gameobjects/Component.h>
#include <src/gameobjects/TransformComponent.h>
#include <src/utilities/NonCopyable.h>
#include <vector>

class GameObject : public NonCopyable
{
public:
    GameObject();
    ~GameObject();

    std::vector<Component*> components;

    template <typename T, typename... Args>
    T* addComponent(Args&&... args);

    template <typename T>
    T* getComponent();

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
