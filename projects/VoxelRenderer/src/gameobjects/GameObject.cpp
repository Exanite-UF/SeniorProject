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

    if (isPartOfWorld)
    {
        Log::error("A GameObject that was part of the world was destroyed without being first removed from the world. Please remove the GameObject from the world first!");
    }

    if (!isRemovalFromWorldComplete)
    {
        Log::error("A GameObject was destroyed while being removed from the world. This means the internal GameObject code was incorrectly implemented!");
    }
}

std::shared_ptr<TransformComponent>& GameObject::getTransform()
{
    assertIsPartOfWorld();

    return transform;
}

bool GameObject::getIsWorldRoot() const
{
    return !transform->hasParent();
}

void GameObject::update()
{
    ZoneScoped;

    assertIsPartOfWorld();

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

    auto gameObject = std::shared_ptr<GameObject>(new GameObject(name));

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

void GameObject::notifyRemovingFromWorld()
{
    ZoneScoped;

    if (wasRemovingFromWorldNotified)
    {
        return;
    }

    wasRemovingFromWorldNotified = true;

    // Notify components in reverse order
    for (int i = components.size() - 1; i >= 0; --i)
    {
        components.at(i)->notifyRemovingFromWorld();
    }
}

void GameObject::actualRemoveFromWorld()
{
    ZoneScoped;

    assertIsPartOfWorld();

    // Remove components in reverse order
    auto componentsCopy = components;
    for (int i = componentsCopy.size() - 1; i >= 0; --i)
    {
        componentsCopy.at(i)->removeFromWorld();
    }

    Assert::isTrue(components.size() == 0, "All components should have been removed at this point");

    // Then remove self
    isPartOfWorld = false;
}

void GameObject::removeFromWorld()
{
    ZoneScoped;

    if (!isPartOfWorld || wasRemoveFromWorldCalled)
    {
        return;
    }

    wasRemoveFromWorldCalled = true;

    // Notify first
    notifyRemovingFromWorld();

    // Then remove self
    actualRemoveFromWorld();
    isRemovalFromWorldComplete = true;
}

bool GameObject::getIsPartOfWorld() const
{
    return isPartOfWorld;
}

void GameObject::assertIsPartOfWorld() const
{
    Assert::isTrue(isPartOfWorld, "GameObject is no longer part of the world");
}

void GameObject::setName(const std::string& name)
{
    this->name = name;
}

const std::string& GameObject::getName()
{
    return name;
}
