#include "SceneComponent.h"
#include <glm/gtx/quaternion.hpp>
#include <src/world/SceneComponent.h>
#include <limits>

#include <glm/glm.hpp>
#include <iostream>

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

void SceneComponent::addObjectChunk(const std::shared_ptr<VoxelChunkComponent>& chunk)
{
    auto iterator = std::find(objectChunks.begin(), objectChunks.end(), chunk);
    if (iterator != objectChunks.end())
    {
        // Already added
        return;
    }

    allChunks.push_back(chunk);
    objectChunks.push_back(chunk);
}

void SceneComponent::removeObjectChunk(const std::shared_ptr<VoxelChunkComponent>& chunk)
{
    std::erase(allChunks, chunk);
    std::erase(objectChunks, chunk);
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

void SceneComponent::setSkybox(const std::shared_ptr<SkyboxComponent>& skybox)
{
    this->skybox = skybox;
}

std::shared_ptr<SkyboxComponent> SceneComponent::getSkybox()
{
    return skybox;
}

std::pair<float, glm::vec3> SceneComponent::raycast(glm::vec3 start, glm::vec3 direction)
{
    glm::vec3 hitLocation;
    float currentDepth = -1;
    for(auto& chunk : allChunks){
        auto result = chunk->raycast(start, direction, currentDepth);

        //std::cout << result.first << " " << result.second.x << " " << result.second.y << " " << result.second.z << std::endl;

        if(currentDepth < 0){
            //If no hit has been found, then the hit only needs to exist
            if(result.first > 0){
                hitLocation = result.second;
                currentDepth = result.first;
            }
        }else{
            //If a hit has been found then it must be the nearest
            if(result.first > 0 && result.first < currentDepth){
                hitLocation = result.second;
                currentDepth = result.first;
            }
        }
        
    }
    return {currentDepth, hitLocation};
}
