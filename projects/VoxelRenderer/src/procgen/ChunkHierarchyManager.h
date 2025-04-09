#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <memory>
#include <mutex>
#include <src/utilities/Singleton.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <src/procgen/data/TreeStructure.h>
#include <src/world/VoxelChunkData.h>

#define sqrt2 1.41421356
class StructureNode
{
public:
    TreeStructure structure;

    StructureNode(TreeStructure structure)
        : structure(structure)
    {
    }
};

class ChunkHierarchyManager : public Singleton<ChunkHierarchyManager>
{
private:
    std::vector<std::unordered_map<glm::ivec2, std::vector<std::shared_ptr<StructureNode>>>> levels;

    // Singleton needs default constructor
    ChunkHierarchyManager()
        : ChunkHierarchyManager(3)
    {
    }

    ChunkHierarchyManager(uint32_t levelCount)
    {
        for (int i = 0; i < levelCount; i++)
        {
            levels.emplace({});
        }
    }

public:
    std::mutex mutex;

    void addStructure(glm::ivec2 chunkPosition, VoxelChunkData& chunkData, glm::vec3 structureOrigin, TreeStructure structure)
    {
        if (levels.size() == 0)
            return;

        glm::ivec2 levelChunkPosition = chunkPosition;
        int chunksPerLevel = 1;

        std::vector<glm::vec3> boundingBoxCornerDirections = {
            { sqrt2, sqrt2, 0 },
            { sqrt2, -sqrt2, 0 },
            { -sqrt2, -sqrt2, 0 },
            { -sqrt2, sqrt2, 0 }
        };

        int selectedLevelIndex = -1;
        for (int i = 0; i < levels.size() - 1; i++)
        {
            auto& level = levels[i];

            float structureRadius = structure.getMaxDistanceFromOrigin();

            int widthPerLevelChunk = chunkData.getSize().x * chunksPerLevel;
            glm::ivec2 levelChunkCornerPosition = levelChunkPosition * widthPerLevelChunk;
            glm::ivec2 levelChunkMinBounds = { levelChunkCornerPosition.x, levelChunkCornerPosition.y };
            glm::ivec2 levelChunkMaxBounds = { levelChunkCornerPosition.x + (chunksPerLevel * widthPerLevelChunk), levelChunkCornerPosition.y + (chunksPerLevel * widthPerLevelChunk) };

            // Case 1: structure is larger than a chunk
            if (2 * structureRadius > widthPerLevelChunk)
            {
                levelChunkPosition = { glm::ceil(levelChunkPosition.x / 2), glm::ceil(levelChunkPosition.y / 2) };
                chunksPerLevel = 2 * chunksPerLevel + 1;
                continue;
            }

            // Case 2: structure is on boundary of chunk
            bool insideBoundingBox = true;
            for (int j = 0; j < boundingBoxCornerDirections.size(); j++)
            {
                glm::vec3& direction = boundingBoxCornerDirections[j];
                glm::ivec3 cornerVoxel = structureOrigin + structureRadius * direction;

                if (cornerVoxel.x <= levelChunkMinBounds.x || cornerVoxel.y <= levelChunkMinBounds.y)
                {
                    insideBoundingBox = false;
                    break;
                }

                if (cornerVoxel.x >= levelChunkMaxBounds.x || cornerVoxel.y >= levelChunkMaxBounds.y)
                {
                    insideBoundingBox = false;
                    break;
                }
            }

            if (!insideBoundingBox)
            {
                levelChunkPosition = { glm::ceil(levelChunkPosition.x / 2), glm::ceil(levelChunkPosition.y / 2) };
                chunksPerLevel = 2 * chunksPerLevel + 1;
                continue;
            }

            selectedLevelIndex = i;
            break;
        }

        // Add structure to level
        if (selectedLevelIndex == -1)
        {
            selectedLevelIndex = levels.size() - 1;
        }

        auto& selectedLevel = levels[selectedLevelIndex];

        if (!selectedLevel.contains(levelChunkPosition))
        {
            std::vector<std::shared_ptr<StructureNode>> empty;
            selectedLevel.emplace(levelChunkPosition, empty);
        }

        std::shared_ptr<StructureNode> node = std::make_shared<StructureNode>(structure);

        auto levelStructures = selectedLevel.find(levelChunkPosition);
        levelStructures->second.push_back(node);
    }

    std::unordered_set<std::shared_ptr<StructureNode>> getStructuresFromChunk(glm::ivec2 chunkPosition)
    {
        std::unordered_set<std::shared_ptr<StructureNode>> allStructures;

        glm::vec2 levelChunkPosition = chunkPosition;

        std::unordered_set<glm::vec2> quad = {
            chunkPosition
        };

        for (int i = 0; i < levels.size(); i++)
        {
            auto& level = levels[i];

            for (auto& mappedChunkPosition : quad)
            {
                auto levelStructures = level.find((glm::ivec2)mappedChunkPosition);

                if (levelStructures != level.end())
                {
                    for (int j = 0; j < levelStructures->second.size(); j++)
                    {
                        allStructures.emplace(levelStructures->second[j]);
                    }
                }
            }

            quad = {
                { glm::floor(levelChunkPosition.x / 2), glm::floor(levelChunkPosition.y / 2) },
                { glm::ceil(levelChunkPosition.x / 2), glm::floor(levelChunkPosition.y / 2) },
                { glm::floor(levelChunkPosition.x / 2), glm::ceil(levelChunkPosition.y / 2) },
                { glm::ceil(levelChunkPosition.x / 2), glm::ceil(levelChunkPosition.y / 2) }
            };

            levelChunkPosition = { glm::ceil(levelChunkPosition.x / 2), glm::ceil(levelChunkPosition.y / 2) };
        }

        return allStructures;
    }
};
