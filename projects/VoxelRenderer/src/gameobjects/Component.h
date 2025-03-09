#pragma once

#include <memory>

#include <src/gameobjects/GameObject.h>
#include <src/gameobjects/TransformComponent.h>
#include <src/utilities/NonCopyable.h>

class GameObject;

class Component : public NonCopyable
{
    friend class GameObject;

protected:
    bool isDestroyed = false;

    // Certain calls are paired
    // If the first event in the pair is called, the second needs to be eventually called:
    // constructor + destructor
    // onCreate + onDestroy

    virtual void onUpdate() {}

    virtual void onCreate() {}
    virtual void onDestroy() {}

private:
    std::shared_ptr<GameObject> gameObject;

public:
    explicit Component();
    ~Component() override;

    void destroy();
    void update();

    std::shared_ptr<TransformComponent>& getTransform();
    std::shared_ptr<GameObject>& getGameObject();

    bool isAlive() const;
};
