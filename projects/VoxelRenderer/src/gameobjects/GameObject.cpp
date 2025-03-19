#include "GameObject.h"

#include <src/utilities/Assert.h>

#include "TransformComponent.h"

GameObject::GameObject() = default;

GameObject::~GameObject()
{
    destroy();
}

std::shared_ptr<TransformComponent>& GameObject::getTransform()
{
    assertIsAlive();

    return transform;
}

std::shared_ptr<GameObject> GameObject::createRootObject()
{
    auto gameObject = std::make_shared<GameObject>();

    // Add default Transform component
    auto transform = gameObject->addComponent<TransformComponent>();

    // Set transform->transform
    // This is required because even though addComponent initializes transform->transform,
    // it doesn't set it to the correct value due to a circular dependency.
    // Doing this avoids that circular dependency.
    transform->transform = transform;

    // Also set gameObject->transform
    gameObject->transform = transform;

    return gameObject;
}

std::shared_ptr<GameObject> GameObject::createChildObject()
{
    auto child = createRootObject();
    child->getTransform()->setParent(getTransform());

    return child;
}

void GameObject::destroy()
{
    assertIsAlive();

    for (const auto& component : components)
    {
        component->destroy();
    }

    isAlive = false;
    components.clear();
}

bool GameObject::getIsAlive() const
{
    return isAlive;
}

void GameObject::assertIsAlive() const
{
    Assert::isTrue(isAlive, "GameObject has been destroyed");
}
