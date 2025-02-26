#pragma once

#include <glm/glm.hpp>
#include <src/gameobjects/Component.h>
#include <src/world/Transform.h>

class TransformComponent : public Component
{
    // Copy from the Transform class

    // vec3 position
    // vec3 rotation
    // vec3 scale

    // onUpdate() override;
    // onCreate() override;
    // onDestroy() override;

public:

    std::shared_ptr<GameObject> gameObject;
    Transform* transform; // don't think we need a shared ptr for this

    TransformComponent(std::shared_ptr<GameObject> parent);
    ~TransformComponent();

    void destroy();

    virtual void onUpdate() = 0;
    virtual void onCreate() = 0;
    virtual void onDestroy() = 0;
};
