#pragma once

#include <glm/glm.hpp>
#include <src/gameobjects/Component.h>
#include <src/gameobjects/GameObject.h>
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
    std::vector<std::shared_ptr<TransformComponent>> children {};

    explicit TransformComponent(std::shared_ptr<GameObject> parent);

    void onDestroy() override;
};
