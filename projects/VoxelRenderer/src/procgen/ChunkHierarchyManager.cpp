#include "ChunkHierarchyManager.h"

#include <src/procgen/data/TreeStructure.h>

#include <glm/glm.hpp>
#include <iostream>
#include <src/procgen/WorldUtility.h>

void ChunkHierarchyManager::validateChunkSize() const
{
    Assert::isTrue(glm::all(glm::greaterThan(chunkSize, glm::ivec3(0))), "Chunk size must be greater that 0. You probably didn't set it.");
}

bool ChunkHierarchyManager::isChunkGenerated(glm::ivec2 chunkPosition, int level)
{
    if (level < 0)
    {
        throw std::runtime_error("Negative levels do not exist.");
    }

    if (level >= isGenerated.size())
    {
        return false;
    }

    glm::ivec2 temp = glm::floor(glm::vec2(chunkPosition) / glm::vec2(chunkSize));

    auto& isLevelGenerated = isGenerated[level];
    auto isChunkGenerated = isLevelGenerated.find(temp);

    // std::cout << (isChunkGenerated != isLevelGenerated.end()) << std::endl;

    if (isChunkGenerated != isLevelGenerated.end())
    {
        if (isChunkGenerated->second)
        {
            // std::cout << temp.x << " " << temp.y << " Rejected" << std::endl;
        }
        else
        {
            // std::cout << temp.x << " " << temp.y << " Accepted" << std::endl;
        }
        return isChunkGenerated->second;
    }
    else
    {
        // std::cout << temp.x << " " << temp.y << " Accepted" << std::endl;
        return false;
    }
}

void ChunkHierarchyManager::setChunkGenerated(glm::ivec2 chunkPosition, int level, bool flag)
{
    if (level < 0)
    {
        throw std::runtime_error("Negative levels do not exist.");
    }

    for (int i = isGenerated.size(); i <= level; i++)
    {
        isGenerated.emplace_back();
    }

    glm::ivec2 temp = glm::floor(glm::vec2(chunkPosition) / glm::vec2(chunkSize));

    std::cout << temp.x << " " << temp.y << " Was made" << std::endl;

    auto& isLevelGenerated = isGenerated[level];
    isLevelGenerated[chunkPosition / glm::ivec2(chunkSize)] = flag;
}

void ChunkHierarchyManager::addStructure(glm::ivec3 structureOrigin, std::shared_ptr<Structure> structure)
{
    validateChunkSize();
    if (levels.size() == 0)
    {
        levels.emplace_back();
        isGenerated.emplace_back();
    }

    glm::ivec2 chunkPosition = glm::floor(glm::vec2(structureOrigin) / glm::vec2(chunkSize));
    glm::ivec2 regionBasePosition = chunkPosition;

    // Start with only the single chunk that this structure is directly in
    std::unordered_set<glm::ivec2> quad = {
        chunkPosition
    };

    glm::vec2 structureRadius = structure->getMaxDistanceFromOrigin();
    std::vector<glm::vec2> boundingBoxCorners = {
        glm::vec2(structureRadius.x, structureRadius.y) + glm::vec2(structureOrigin),
        glm::vec2(structureRadius.x, -structureRadius.y) + glm::vec2(structureOrigin),
        glm::vec2(-structureRadius.x, -structureRadius.y) + glm::vec2(structureOrigin),
        glm::vec2(-structureRadius.x, structureRadius.y) + glm::vec2(structureOrigin)
    };
    // std::cout << "Structure Radius: " << structureRadius.x << " " << structureRadius.y << std::endl;

    int levelCounter = 0;
    while (true)
    {
        levelsMTX.lock();
        auto& level = levels[levelCounter]; // Get the current level
        auto& isGeneratedLevel = isGenerated[levelCounter];

        glm::vec2 radiusOfRegion = glm::vec2(chunkSize) * (powf(2, levelCounter - 1) + 0.5f);
        if (levelCounter == 0)
        {
            radiusOfRegion = glm::vec2(chunkSize) / 2.f;
        }
        
        // std::cout << "Radius of region: " << levelCounter << " " << radiusOfRegion.x << " " << radiusOfRegion.y << std::endl;

        // Case 1: structure is larger than a chunk
        // If so, then it is unable to fit in any of the chunks of this level
        if (glm::any(glm::greaterThan(structureRadius, radiusOfRegion)))
        {
            // So proceed to the next level
            levelsMTX.unlock();

            levelCounter++;

            // If the next level does not exist, create it
            if (levels.size() == levelCounter)
            {
                levels.emplace_back();
                isGenerated.emplace_back();
            }

            // Coordinate adjustment for the next level
            regionBasePosition = glm::floor(glm::vec2(regionBasePosition) / 2.f); // I need to verify that floor(chunkPosition / pow(2, i)) == That but flooring after every divide by 2
            quad = {
                regionBasePosition + glm::ivec2(0, 0),
                regionBasePosition + glm::ivec2(1, 0),
                regionBasePosition + glm::ivec2(0, 1),
                regionBasePosition + glm::ivec2(1, 1)
            };
            continue;
        }

        bool wasAdded = false;
        for (auto& regionIndex : quad)
        {
            glm::vec2 center = glm::vec2(chunkSize) * (powf(2.0, levelCounter) * glm::vec2(regionIndex) + glm::vec2(0.5));

            // std::cout << "Center: " << center.x << " " << center.y << std::endl;

            // Case 2: Is the structure found entirely inside this region

            bool allInside = true;
            for (int j = 0; j < boundingBoxCorners.size(); j++)
            {

                glm::vec2& corner = boundingBoxCorners[j];

                // std::cout << "Distance: " << glm::abs(corner - center).x << " " << glm::abs(corner - center).y << std::endl;
                // If the bounding box corner is inside the region along all axes, then the corner is inside the region
                if (!glm::all(glm::lessThanEqual(glm::abs(corner - center), glm::vec2(radiusOfRegion))))
                {
                    allInside = false;
                }
            }

            // std::cout << "Result: " << allInside << std::endl;

            // If all the corners are not inside the region, then do not add it to this region
            if (!allInside)
            {
                continue;
            }

            // At this point the structure is guaranteed to fit inside the region, so add it
            wasAdded = true;

            level[regionIndex].push_back(structure);
            // if(!isGeneratedLevel.count(regionIndex)){
            //     isGeneratedLevel[regionIndex] = false;
            // }
        }
        // If it was not added at this level, then try at the next level
        levelsMTX.unlock();

        // If it was added, stop
        if (wasAdded)
        {
            break;
        }

        // If is was not added, progress to the next level
        levelCounter++;

        // If the next level does not exist, create it
        if (levels.size() == levelCounter)
        {
            levels.emplace_back();
            isGenerated.emplace_back();
        }

        // Coordinate adjustment for the next level
        regionBasePosition = glm::floor(glm::vec2(regionBasePosition) / 2.f); // I need to verify that floor(chunkPosition / pow(2, i)) == That but flooring after every divide by 2
        quad = {
            regionBasePosition + glm::ivec2(0, 0),
            regionBasePosition + glm::ivec2(1, 0),
            regionBasePosition + glm::ivec2(0, 1),
            regionBasePosition + glm::ivec2(1, 1)
        };
    }
}

std::unordered_set<std::shared_ptr<Structure>> ChunkHierarchyManager::getStructuresForChunk(glm::ivec2 chunkPosition)
{
    validateChunkSize();
    std::unordered_set<std::shared_ptr<Structure>> allStructures;

    glm::ivec2 regionBasePosition = glm::floor(glm::vec2(chunkPosition) / glm::vec2(chunkSize));

    // Start with only the single chunk that this structure is directly in
    std::unordered_set<glm::ivec2> quad = {
        regionBasePosition
    };

    // std::cout << levels.size() << std::endl;
    // For all the levels that exist
    for (int i = 0; i < levels.size(); i++)
    {
        levelsMTX.lock_shared();
        auto& level = levels[i];
        // For this level, check all the regions that need to be searched
        for (auto& regionIndex : quad)
        {
            // Look for region data corresponding to the target position to be searched
            auto levelStructures = level.find(regionIndex);

            // std::cout << "CHECK: " << i << " " << regionIndex.x << " " << regionIndex.y << std::endl;

            // If data exists, copy all that data into the set of structures that were found
            if (levelStructures != level.end())
            {
                for (int j = 0; j < levelStructures->second.size(); j++)
                {
                    allStructures.insert(levelStructures->second[j]);
                }
            }
        }

        // Coordinate adjustment for the next level
        regionBasePosition = glm::floor(glm::vec2(regionBasePosition) / 2.f); // I need to verify that floor(chunkPosition / pow(2, i)) == That but flooring after every divide by 2
        quad = {
            regionBasePosition + glm::ivec2(0, 0),
            regionBasePosition + glm::ivec2(1, 0),
            regionBasePosition + glm::ivec2(0, 1),
            regionBasePosition + glm::ivec2(1, 1)
        };
        levelsMTX.unlock_shared();
    }

    return allStructures;
}

void ChunkHierarchyManager::clear()
{
    levelsMTX.lock();
    levels.clear();
    isGenerated.clear();
    levelsMTX.unlock();
}

void ChunkHierarchyManager::setChunkSize(glm::ivec3 size)
{
    this->chunkSize = size;
}
