#pragma once

#include <memory>

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

    virtual void onUpdate() { }

    virtual void onCreate() { }
    virtual void onDestroy() { }

public:
    std::shared_ptr<GameObject> gameObject;

    explicit Component();
    ~Component() override;

    void destroy();
    void update();

    bool isAlive() const;
};
