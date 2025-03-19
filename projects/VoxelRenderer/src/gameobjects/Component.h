#pragma once

#include <memory>

#include <src/utilities/NonCopyable.h>

class GameObject;
class TransformComponent;

class Component : public NonCopyable
{
    friend class GameObject;

protected:
    bool isAlive = true;

    // Certain calls are paired
    // If the first event in the pair is called, the second will eventually called:
    // constructor + destructor
    // onCreate + onDestroy

    Component();
    ~Component() override;

    virtual void onUpdate();

    virtual void onCreate();
    virtual void onDestroy();

private:
    std::shared_ptr<GameObject> gameObject;
    std::shared_ptr<TransformComponent> transform;

public:
    void destroy();
    void update();

    std::shared_ptr<TransformComponent>& getTransform();
    std::shared_ptr<GameObject>& getGameObject();

    bool getIsAlive() const;
    void assertIsAlive() const;
};
