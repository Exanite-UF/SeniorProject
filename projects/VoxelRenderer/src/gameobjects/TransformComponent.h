#pragma once

#include <src/gameobjects/Component.h>
#include <glm/glm.hpp>

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
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    TransformComponent(GameObject* gameObject);
    
    void onUpdate() override;
    void onCreate() override;
    void onDestroy() override;
};
