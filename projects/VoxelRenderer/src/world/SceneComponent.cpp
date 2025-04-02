#include <glm/gtx/quaternion.hpp>
#include <src/world/SceneComponent.h>

std::shared_mutex& SceneComponent::getMutex()
{
    return mutex;
}

void SceneComponent::setCamera(const std::shared_ptr<CameraComponent>& camera)
{
    this->camera = camera;
}

const std::shared_ptr<CameraComponent>& SceneComponent::getCamera()
{
    return camera;
}

const std::vector<std::shared_ptr<VoxelChunkComponent>>& SceneComponent::getAllChunks()
{
    return allChunks;
}

const std::vector<std::shared_ptr<VoxelChunkComponent>>& SceneComponent::getObjectChunks()
{
    return objectChunks;
}

const std::vector<std::shared_ptr<VoxelChunkComponent>>& SceneComponent::getWorldChunks()
{
    return worldChunks;
}

bool SceneComponent::tryGetWorldChunkAtPosition(const glm::ivec3& chunkPosition, std::shared_ptr<VoxelChunkComponent>& outResult)
{
    auto iterator = worldChunksByChunkPosition.find(chunkPosition);
    if (iterator == worldChunksByChunkPosition.end())
    {
        return false;
    }

    outResult = iterator->second;

    return true;
}

bool SceneComponent::tryGetClosestWorldChunk(std::shared_ptr<VoxelChunkComponent>& outResult)
{
    auto cameraPosition = camera->getTransform()->getGlobalPosition();

    outResult = {};
    float closestSquareDistance = std::numeric_limits<float>::infinity();
    for (auto& chunk : worldChunks)
    {
        auto offset = chunk->getTransform()->getGlobalPosition() - cameraPosition;
        auto distance = glm::length2(offset);

        if (distance < closestSquareDistance)
        {
            outResult = chunk;
            closestSquareDistance = distance;
        }
    }

    return !!outResult;
}

void SceneComponent::addWorldChunk(const glm::ivec3& chunkPosition, std::shared_ptr<VoxelChunkComponent>& chunk)
{
    if (worldChunksByChunkPosition.emplace(chunkPosition, chunk).second)
    {
        allChunks.push_back(chunk);
        worldChunks.push_back(chunk);
    }
}

void SceneComponent::removeWorldChunk(const glm::ivec3& chunkPosition)
{
    // Find chunk in chunksByChunkPosition and erase it
    auto mapIterator = worldChunksByChunkPosition.find(chunkPosition);
    if (mapIterator != worldChunksByChunkPosition.end())
    {
        auto chunk = mapIterator->second;
        worldChunksByChunkPosition.erase(mapIterator);

        std::erase(allChunks, chunk);
        std::erase(worldChunks, chunk);
    }
}
