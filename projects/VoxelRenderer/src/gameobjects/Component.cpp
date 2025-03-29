#include "Component.h"

#include <format>

#include <src/Constants.h>
#include <src/gameobjects/GameObject.h>
#include <src/utilities/Assert.h>
#include <src/utilities/Log.h>
#include <src/utilities/PointerUtility.h>
#include <tracy/Tracy.hpp>

Component::Component()
{
    ZoneScoped;

    if constexpr (Constants::GameObject::isEventLoggingEnabled)
    {
        Log::verbose(std::format("Component constructor called for {:s} at @{:x}", typeid(*this).name(), reinterpret_cast<uintptr_t>(this)));
    }
}

Component::~Component()
{
    ZoneScoped;

    if constexpr (Constants::GameObject::isEventLoggingEnabled)
    {
        Log::verbose(std::format("Component destructor called for {:s} @{:x}", typeid(*this).name(), reinterpret_cast<uintptr_t>(this)));
    }

    if (isPartOfWorld)
    {
        Log::error("A Component that was part of the world was destroyed without being first removed from the world. Please remove the Component from the world first!");
    }

    if (!isRemovalFromWorldComplete)
    {
        Log::error("A Component was destroyed while being removed from the world. This means the internal Component code was incorrectly implemented!");
    }
}

void Component::onAddedToWorld()
{
    ZoneScoped;

    if constexpr (Constants::GameObject::isEventLoggingEnabled)
    {
        Log::verbose(std::format("Component::onAddedToWorld called called for '{:s}' ({:s}) at @{:x}", getGameObject()->getName(), typeid(*this).name(), reinterpret_cast<uintptr_t>(this)));
    }
}

void Component::onRemovingFromWorld()
{
    ZoneScoped;

    if constexpr (Constants::GameObject::isEventLoggingEnabled)
    {
        Log::verbose(std::format("Component::onRemovingFromWorld called called for '{:s}' ({:s}) at @{:x}", getGameObject()->getName(), typeid(*this).name(), reinterpret_cast<uintptr_t>(this)));
    }
}

void Component::onUpdate()
{
    ZoneScoped;

    if constexpr (Constants::GameObject::isEventLoggingEnabled && Constants::GameObject::isUpdateEventLoggingEnabled)
    {
        Log::verbose(std::format("Component::onUpdate called called for '{:s}' ({:s}) at @{:x}", getGameObject()->getName(), typeid(*this).name(), reinterpret_cast<uintptr_t>(this)));
    }
}

void Component::notifyAddedToWorld()
{
    ZoneScoped;

    if (wasAddedToWorldNotified)
    {
        return;
    }

    wasAddedToWorldNotified = true;
    onAddedToWorld();
}

void Component::notifyRemovingFromWorld()
{
    ZoneScoped;

    if (wasRemovingFromWorldNotified)
    {
        return;
    }

    wasRemovingFromWorldNotified = true;

    if (!wasAddedToWorldNotified)
    {
        Log::verbose("Skipping Component::onRemovingFromWorld call since Component::onAddedToWorld was not called");

        return;
    }

    wasAddedToWorldNotified = false;

    // onDestroy is user facing so we should only call it when onCreate was also called
    onRemovingFromWorld();
}

void Component::notifyUpdate()
{
    ZoneScoped;

    onUpdate();
}

void Component::actualRemoveFromWorld()
{
    ZoneScoped;

    assertIsPartOfWorld();

    // Remove self from GameObject
    std::erase(getGameObject()->components, shared_from_this());

    isPartOfWorld = false;
}

void Component::removeFromWorld()
{
    ZoneScoped;

    if (!isPartOfWorld || wasRemoveFromGameObjectCalled)
    {
        return;
    }

    wasRemoveFromGameObjectCalled = true;

    // Notify first
    notifyRemovingFromWorld();

    // Then destroy self
    actualRemoveFromWorld();
    isRemovalFromWorldComplete = true;
}

std::shared_ptr<TransformComponent> Component::getTransform() const
{
    assertIsPartOfWorld();

    return PointerUtility::safeLock(transform, "Transform pointer has expired");
}

std::shared_ptr<GameObject> Component::getGameObject() const
{
    assertIsPartOfWorld();

    return PointerUtility::safeLock(gameObject, "GameObject pointer has expired");
}

bool Component::getIsPartOfWorld() const
{
    return isPartOfWorld;
}

void Component::assertIsPartOfWorld() const
{
    Assert::isTrue(isPartOfWorld, "Component is no longer part of the world");
}
