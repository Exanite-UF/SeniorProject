#include "Component.h"

#include <src/utilities/Assert.h>
#include <src/utilities/Log.h>

#include "GameObject.h"

Component::Component()
{
    if constexpr (isEventLoggingEnabled)
    {
        Log::log(std::string("Component constructor called for ") + typeid(*this).name());
    }
}

Component::~Component()
{
    if constexpr (isEventLoggingEnabled)
    {
        Log::log(std::string("Component constructor called for ") + typeid(*this).name());
    }

    destroy();
}

void Component::onCreate()
{
    if constexpr (isEventLoggingEnabled)
    {
        Log::log("Component::onCreate called for '" + getGameObject()->getName() + "' (" + typeid(*this).name() + ")");
    }
}

void Component::onDestroy()
{
    if constexpr (isEventLoggingEnabled)
    {
        Log::log("Component::onDestroy called for '" + getGameObject()->getName() + "' (" + typeid(*this).name() + ")");
    }
}

void Component::onUpdate()
{
    if constexpr (isEventLoggingEnabled && isUpdateEventLoggingEnabled)
    {
        Log::log("Component::onUpdate called for '" + getGameObject()->getName() + "' (" + typeid(*this).name() + ")");
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
    isAlive = false;
    gameObject.reset();
    transform.reset();

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
