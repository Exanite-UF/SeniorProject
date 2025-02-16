#pragma once

#include <src/utilities/NonCopyable.h>

class Component : public NonCopyable
{
    // std::shared_ptr<GameObject> gameObject;
    // GameObject*, I think we want to use a raw pointer to avoid the circular reference counter issue.
    // Also, it makes sense that the GameObjects owns the lifecycle of the Components it has.

    // void destroy();

    // Might not need:
    // virtual void onCreate(); - Similar to constructor, but we call these on the next update
    // virtual void onDestroy(); - Similar to destructor, but called right before

    // Definitely need:
    // virtual void onUpdate();

    // Paired calls:
    // constructor + destructor
    // onCreate + onDestroy
    // If the first event in the pair is called, the second needs to be eventually called.
};
