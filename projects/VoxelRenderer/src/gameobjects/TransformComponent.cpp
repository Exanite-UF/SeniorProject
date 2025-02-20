#include "TransformComponent.h"

TransformComponent::TransformComponent(GameObject* gameObject) : Component(gameObject)
{
    position = glm::vec3(0.0f);
    rotation = glm::vec3(0.0f);
    scale = glm::vec3(1.0f);
}

// not sure if the following are needed but will keep for now for consistency
void TransformComponent::onCreate()
{

}

void TransformComponent::onUpdate()
{
    
}

void TransformComponent::onDestroy()
{
    
}