#include "GameObject.h"

#include <format>

#include <src/Constants.h>
#include <src/gameobjects/TransformComponent.h>
#include <src/utilities/Assert.h>
#include <src/utilities/Log.h>
#include <tracy/Tracy.hpp>

GameObject::GameObject(const std::string& name)
{
    ZoneScoped;

    if constexpr (Constants::GameObject::isEventLoggingEnabled)
    {
        Log::information(std::format("GameObject constructor called for {:s} at @{:x}", typeid(*this).name(), reinterpret_cast<uintptr_t>(this)));
    }

    this->name = name;
}

GameObject::~GameObject()
{
    ZoneScoped;

    if constexpr (Constants::GameObject::isEventLoggingEnabled)
    {
        Log::information(std::format("GameObject destructor called for {:s} at @{:x}", typeid(*this).name(), reinterpret_cast<uintptr_t>(this)));
    }
}

std::shared_ptr<TransformComponent>& GameObject::getTransform()
{
    assertIsAlive();

    return transform;
}

void GameObject::update()
{
    ZoneScoped;

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
    ZoneScoped;

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
    ZoneScoped;

    auto child = createRootObject(name);
    child->getTransform()->setParent(getTransform());

    return child;
}

void GameObject::notifyDestroy()
{
    ZoneScoped;

    if (wasDestroyNotified)
    {
        return;
    }

    wasDestroyNotified = true;

    // Notify components in reverse order
    for (int i = components.size() - 1; i >= 0; --i)
    {
        components.at(i)->notifyDestroy();
    }
}

void GameObject::actualDestroy()
{
    ZoneScoped;

    assertIsAlive();

    isAlive = false;

    // Destroy components in reverse order
    auto componentsCopy = components;
    for (int i = componentsCopy.size() - 1; i >= 0; --i)
    {
        componentsCopy.at(i)->destroy();
    }

    transform.reset();
    components.clear();
    componentsCopy.clear();

    // Then destroy self
    isDestroyComplete = true;
}

void GameObject::destroy()
{
    ZoneScoped;

    if (!isAlive || wasPublicDestroyCalled)
    {
        return;
    }

    wasPublicDestroyCalled = true;

    // Notify first
    notifyDestroy();

    // Then destroy self
    actualDestroy();
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
