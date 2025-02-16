#pragma once

#include <src/utilities/NonCopyable.h>

class GameObject : public NonCopyable
{
    // std::vector<std::shared_ptr<Components>> components;

    // components.at(0) should be the built-in TransformComponent

    // template <class T>
    // getComponent<T>
    // addComponent<T>

    // gameObject.destroy()
    // component.destroy()
    // destroy() should respect the transform hierarchy
};
