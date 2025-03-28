#include "Component.h"

#include <format>

#include <src/Constants.h>
#include <src/gameobjects/GameObject.h>
#include <src/utilities/Assert.h>
#include <src/utilities/Log.h>
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
}

void Component::onCreate()
{
    ZoneScoped;

    if constexpr (Constants::GameObject::isEventLoggingEnabled)
    {
        Log::verbose(std::format("Component::onCreate called called for '{:s}' ({:s}) at @{:x}", getGameObject()->getName(), typeid(*this).name(), reinterpret_cast<uintptr_t>(this)));
    }
}

void Component::onDestroy()
{
    ZoneScoped;

    if constexpr (Constants::GameObject::isEventLoggingEnabled)
    {
        Log::verbose(std::format("Component::onDestroy called called for '{:s}' ({:s}) at @{:x}", getGameObject()->getName(), typeid(*this).name(), reinterpret_cast<uintptr_t>(this)));
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

void Component::notifyCreate()
{
    ZoneScoped;

    if (wasCreateNotified)
    {
        return;
    }

    wasCreateNotified = true;
    onCreate();
}

void Component::notifyDestroy()
{
    ZoneScoped;

    if (wasDestroyNotified)
    {
        return;
    }

    wasDestroyNotified = true;

    if (!wasCreateNotified)
    {
        Log::verbose("Skipping Component::onDestroy call since Component::onCreate was not called");

        return;
    }

    wasCreateNotified = false;

    // onDestroy is user facing so we should only call it when onCreate was also called
    onDestroy();
}

void Component::notifyUpdate()
{
    ZoneScoped;

    onUpdate();
}

void Component::actualDestroy()
{
    ZoneScoped;

    assertIsAlive();

    isAlive = false;

    // Remove self from GameObject
    std::erase(gameObject->components, shared_from_this());

    // Destroy self
    gameObject.reset();
    transform.reset();

    isDestroyComplete = true;
}

void Component::destroy()
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

std::shared_ptr<TransformComponent>& Component::getTransform()
{
    assertIsAlive();

    return transform;
}

std::shared_ptr<GameObject>& Component::getGameObject()
{
    assertIsAlive();

    return gameObject;
}

bool Component::getIsAlive() const
{
    return isAlive;
}

void Component::assertIsAlive() const
{
    Assert::isTrue(isAlive, "Component has been destroyed");
}
