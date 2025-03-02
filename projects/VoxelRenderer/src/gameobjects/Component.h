#pragma once

#include <src/gameobjects/GameObject.h>
#include <src/utilities/NonCopyable.h>

class Component : public NonCopyable
{
    // std::shared_ptr<GameObject> gameObject;
    // GameObject*, I think we want to use a raw pointer to avoid the circular reference counter issue.
    // Also, it makes sense that the GameObjects owns the lifecycle of the Components it has.

public:
    std::shared_ptr<GameObject> gameObject;
    Component(std::shared_ptr<GameObject> parent);
    ~Component();

    void destroy();

    virtual void onUpdate() = 0;
    virtual void onCreate() = 0;
    virtual void onDestroy() = 0;

    // Paired calls:
    // constructor + destructor
    // onCreate + onDestroy
    // If the first event in the pair is called, the second needs to be eventually called.
};
