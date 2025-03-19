#include "GameObject.h"

#include <src/utilities/Assert.h>

#include "TransformComponent.h"

GameObject::GameObject(const std::string& name)
{
    this->name = name;
}

GameObject::~GameObject()
{
    destroy();
}

std::shared_ptr<TransformComponent>& GameObject::getTransform()
{
    assertIsAlive();

    return transform;
}

void GameObject::update()
{
    assertIsAlive();

    // Update own components
    for (auto component : components)
    {
        component->notifyUpdate();
    }

    // Then update children
    for (auto child : transform->getChildren())
    {
        child->getGameObject()->update();
    }
}

std::shared_ptr<GameObject> GameObject::createRootObject(const std::string& name)
{
    auto gameObject = std::make_shared<GameObject>(name);

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

std::shared_ptr<GameObject> GameObject::createChildObject(const std::string& name)
{
    auto child = createRootObject(name);
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

void GameObject::setName(const std::string& name)
{
    this->name = name;
}

const std::string& GameObject::getName()
{
    return name;
}
