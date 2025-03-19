#include "Component.h"

#include <src/utilities/Assert.h>
#include <src/utilities/Log.h>

Component::Component() = default;

Component::~Component()
{
    destroy();
}

void Component::onUpdate()
{
    Log::log("Component::onUpdate");
}

void Component::onCreate()
{
    Log::log("Component::onCreate");
}

void Component::onDestroy()
{
    Log::log("Component::onDestroy");
}

void Component::destroy()
{
    assertIsAlive();

    onDestroy();

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
