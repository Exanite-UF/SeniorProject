#include <src/world/SceneComponent.h>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

bool SceneComponent::tryGetClosestChunk(std::shared_ptr<VoxelChunkComponent>& result)
{
    auto cameraPosition = camera->getTransform()->getGlobalPosition();

    result = {};
    float closestSquareDistance = std::numeric_limits<float>::infinity();
    for (auto& chunk : chunks)
    {
        auto offset = chunk->getTransform()->getGlobalPosition() - cameraPosition;
        auto distance = glm::length2(offset);

        if (distance < closestSquareDistance)
        {
            result = chunk;
            closestSquareDistance = distance;
        }
    }

    return !!result;
}
