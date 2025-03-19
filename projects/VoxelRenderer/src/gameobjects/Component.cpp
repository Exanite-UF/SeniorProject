#include "Component.h"

#include <src/Constants.h>
#include <src/utilities/Assert.h>
#include <src/utilities/Log.h>

#include "GameObject.h"

Component::Component()
{
    if constexpr (Constants::GameObject::isEventLoggingEnabled)
    {
        Log::log(std::format("Component constructor called for {:s} at @{:x}", typeid(*this).name(), reinterpret_cast<uintptr_t>(this)));
    }
}

Component::~Component()
{
    if constexpr (Constants::GameObject::isEventLoggingEnabled)
    {
        Log::log(std::format("Component destructor called for {:s} @{:x}", typeid(*this).name(), reinterpret_cast<uintptr_t>(this)));
    }
}

void Component::onCreate()
{
    if constexpr (Constants::GameObject::isEventLoggingEnabled)
    {
        Log::log(std::format("Component::onCreate called called for '{:s}' ({:s}) at @{:x}", getGameObject()->getName(), typeid(*this).name(), reinterpret_cast<uintptr_t>(this)));
    }
}

void Component::onDestroy()
{
    if constexpr (Constants::GameObject::isEventLoggingEnabled)
    {
        Log::log(std::format("Component::onDestroy called called for '{:s}' ({:s}) at @{:x}", getGameObject()->getName(), typeid(*this).name(), reinterpret_cast<uintptr_t>(this)));
    }
}

void Component::onUpdate()
{
    if constexpr (Constants::GameObject::isEventLoggingEnabled && Constants::GameObject::isUpdateEventLoggingEnabled)
    {
        Log::log(std::format("Component::onUpdate called called for '{:s}' ({:s}) at @{:x}", getGameObject()->getName(), typeid(*this).name(), reinterpret_cast<uintptr_t>(this)));
    }
}

void Component::notifyCreate()
{
    if (wasCreateCalled)
    {
        return;
    }

    wasCreateCalled = true;
    onCreate();
}

void Component::notifyDestroy()
{
    if (isDestroyPending)
    {
        return;
    }

    isDestroyPending = true;

    if (!wasCreateCalled)
    {
        return;
    }

    wasCreateCalled = false;

    onDestroy();
}

void Component::notifyUpdate()
{
    onUpdate();
}

void Component::actualDestroy()
{
    assertIsAlive();

    // Destroy self
    gameObject.reset();
    transform.reset();

    isAlive = false;
    isDestroyPending = false;
}

void Component::destroy()
{
    if (!isAlive || isDestroyPending)
    {
        return;
    }

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
