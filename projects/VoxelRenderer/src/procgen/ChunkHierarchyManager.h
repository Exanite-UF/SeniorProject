#pragma once

#include <glm/glm.hpp>
#include <src/utilities/Singleton.h>

class StructureNode
{
};

class ChunkHierarchyManager : public Singleton<ChunkHierarchyManager>
{
    // Chunk0 position: [3]
    // Chunk1 position: 3   / 2   -> 1.5  | floor(1.5), ceil(1.5) -> 1, [2]
    // Chunk2 position: 2   / 2   -> 1    | floor(1)  , ceil(1)   -> [1]

    // Chunk0 position: [5]
    // Chunk1 position: 5   / 2   -> 2.5  | floor(2.5), ceil(2.5) -> 2, [3]
    // Chunk2 position: 3   / 2   -> 1.5  | floor(1.5), ceil(1.5) -> 1, 2

private:
    std::vector<std::unordered_map<int, std::vector<StructureNode>>> chunks;

public:
    std::vector<StructureNode> getStructuresForChunkPosition(int chunkPosition)
    {
        std::vector<StructureNode> results {};
        float chunkPositionF = chunkPosition;

        for (int i = 0; i < chunks.size(); i++)
        {
            int first = glm::floor(chunkPositionF / 2);
            int second = glm::ceil(chunkPositionF / 2);

            auto firstIterator = chunks.at(i).find(first);
            if (firstIterator != chunks.at(i).end())
            {
                results.insert(results.end(), firstIterator->second.begin(), firstIterator->second.end());
            }

            auto secondIterator = chunks.at(i).find(second);
            if (secondIterator != chunks.at(i).end())
            {
                results.insert(results.end(), secondIterator->second.begin(), secondIterator->second.end());
            }

            chunkPositionF = glm::ceil(chunkPositionF / 2);
        }

        return results;
    };
};
