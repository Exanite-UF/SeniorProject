#pragma once

#include <memory>
#include <src/world/CameraComponent.h>

struct CameraRenderCommand
{
    std::shared_ptr<CameraComponent> camera;
};
