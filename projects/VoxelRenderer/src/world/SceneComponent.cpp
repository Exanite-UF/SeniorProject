#include <src/world/SceneComponent.h>

#include <glm/gtx/quaternion.hpp>

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

const std::vector<std::shared_ptr<VoxelChunkComponent>>& SceneComponent::getChunks()
{
    return chunks;
}

const std::vector<std::shared_ptr<VoxelChunkComponent>>& SceneComponent::getVisibleChunks()
{
    return visibleChunks;
}

bool SceneComponent::tryGetChunkAtPosition(const glm::ivec3& chunkPosition, std::shared_ptr<VoxelChunkComponent>& result)
{
    auto iterator = chunksByChunkPosition.find(chunkPosition);
    if (iterator == chunksByChunkPosition.end())
    {
        return false;
    }

    result = iterator->second;

    return true;
}

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

void SceneComponent::addChunk(const glm::ivec3& chunkPosition, std::shared_ptr<VoxelChunkComponent>& chunk)
{
    if (chunksByChunkPosition.emplace(chunkPosition, chunk).second)
    {
        // If emplace succeeds, also add it to the flattened vector
        chunks.push_back(chunk);
    }
}

void SceneComponent::removeChunk(const glm::ivec3& chunkPosition)
{
    // Find chunk in chunksByChunkPosition and erase it
    auto mapIterator = chunksByChunkPosition.find(chunkPosition);
    if (mapIterator != chunksByChunkPosition.end())
    {
        auto chunk = mapIterator->second;
        chunksByChunkPosition.erase(mapIterator);

        // If removal from chunksByChunkPosition succeeds, also remove it from the flattened vector
        auto vectorIterator = std::find(chunks.begin(), chunks.end(), chunk);
        if (vectorIterator != chunks.end())
        {
            chunks.erase(vectorIterator);
        }
    }
}
