#pragma once

#include <atomic>
#include <memory>

#include <src/utilities/CountInstances.h>
#include <src/utilities/NonCopyable.h>

class GameObject;
class TransformComponent;

class Component : public NonCopyable, public CountInstances
{
    friend class GameObject;

private:
    std::shared_ptr<GameObject> gameObject;
    std::shared_ptr<TransformComponent> transform;

    // Components aren't supposed to be thread safe, but this provides a bit of extra safety
    std::atomic<bool> isAlive = true;
    std::atomic<bool> wasCreateNotified = false;
    std::atomic<bool> wasPublicDestroyCalled = false;
    std::atomic<bool> wasDestroyNotified = false;
    std::atomic<bool> isDestroyComplete = false;

    void notifyCreate();

    void notifyDestroy();

    void notifyUpdate();

    void actualDestroy();

protected:
    Component();

    ~Component() override;

    // Certain calls are paired
    // If the first event in the pair is called, the second will eventually called:
    // constructor + destructor
    // onCreate + onDestroy

    // Note: A component is not considered destroyed until after onDestroy is called, specifically it is not destroyed until the internal actualDestroy call.
    virtual void onCreate();

    virtual void onDestroy();

    virtual void onUpdate();

public:
    // When destroy is called, all components that will be destroyed will be first notified
    // of their pending destruction.
    //
    // Only after all components are notified will any components be actually destroyed.
    //
    // Internally, this is done by doing the following:
    // 1. destroy() will call notifyDestroy() to notify internal code of the destruction.
    //    This method call is propagated throughout the GameObject/Component hierarchy.
    // 2. notifyDestroy() will call onDestroy() to notify user code of the destruction.
    // 3. destroy() will then call actualDestroy(). This method does the actual internal cleanup logic.
    //
    // If destroy is recursively called during this process, that destroy call will also do the same logic as above.
    // This means for some components notifyDestroy() and destroy() will be called multiple times.
    // However, these methods both are idempotent.
    // Regardless of how many times they are called, onDestroy and actualDestroy will only be called once.
    //
    // Summary of methods used:
    // destroy() - Called by user. Notifies components of destruction.
    // notifyDestroy() - Called internally. Notifies components of destruction.
    // onDestroy() - User facing callback. Used to run user-facing cleanup logic.
    // actualDestroy() - Internal facing callback. Used to run internal cleanup logic.
    //
    // destroy() is implemented on both GameObjects and Components.
    // notifyDestroy is implemented on both GameObjects and Components.
    // onDestroy() is Component only. This is because users are only allowed to extend from the Component class.
    // actualDestroy is implemented on both GameObjects and Components.

    void destroy();

    std::shared_ptr<TransformComponent>& getTransform();

    std::shared_ptr<GameObject>& getGameObject();

    bool getIsAlive() const;

    void assertIsAlive() const;
};
