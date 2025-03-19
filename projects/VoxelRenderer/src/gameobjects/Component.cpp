#include "Component.h"

#include <src/utilities/Assert.h>
#include <src/utilities/Log.h>

#include "GameObject.h"

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

Component::Component() = default;

Component::~Component()
{
    destroy();
}

void Component::onCreate()
{
    Log::log("Component::onCreate for '" + getGameObject()->getName() + "' (" + typeid(*this).name() + ")");
}

void Component::onDestroy()
{
    Log::log("Component::onDestroy for '" + getGameObject()->getName() + "' (" + typeid(*this).name() + ")");
}

void Component::onUpdate()
{
    Log::log("Component::onUpdate for '" + getGameObject()->getName() + "' (" + typeid(*this).name() + ")");
}

void Component::destroy()
{
    assertIsAlive();

    notifyDestroy();

    isAlive = false;
    gameObject.reset();
    transform.reset();
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
