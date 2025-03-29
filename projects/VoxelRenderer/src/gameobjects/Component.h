#pragma once

#include <atomic>
#include <memory>

#include <src/utilities/CountInstances.h>
#include <src/utilities/NonCopyable.h>

class GameObject;
class TransformComponent;

class Component : public NonCopyable, public CountInstances<Component>, public std::enable_shared_from_this<Component>
{
    friend class GameObject;

private:
    std::weak_ptr<GameObject> gameObject;
    std::weak_ptr<TransformComponent> transform;

    // Components aren't supposed to be thread safe, but atomic provides a bit of extra safety
    std::atomic<bool> isPartOfWorld = true;

    std::atomic<bool> wasAddedToWorldNotified = false;
    std::atomic<bool> wasRemovingFromWorldNotified = false;

    std::atomic<bool> wasRemoveFromGameObjectCalled = false;
    std::atomic<bool> isRemovalFromWorldComplete = false;

    void notifyAddedToWorld();
    void notifyRemovingFromWorld();
    void notifyUpdate();
    void actualRemoveFromWorld();

protected:
    Component();

    ~Component() override;

    // Certain calls are paired
    // If the first event in the pair is called, the second will eventually called:
    // constructor + destructor
    // onAddedToWorld + onRemovingFromWorld

    virtual void onAddedToWorld();
    virtual void onRemovingFromWorld();
    virtual void onUpdate();

public:
    void removeFromWorld();

    std::shared_ptr<TransformComponent> getTransform() const;
    std::shared_ptr<GameObject> getGameObject() const;

    bool getIsPartOfWorld() const;
    void assertIsPartOfWorld() const;
};
