#pragma once

#include <glm/vec3.hpp>
#include <memory>
#include <src/gameobjects/GameObject.h>

struct RaycastHit
{
public:
    bool isValid = false;
    float distance = 0;
    glm::vec3 position = glm::vec3(0);
    std::shared_ptr<GameObject> object {};

    explicit RaycastHit();
    static RaycastHit invalid();
};
