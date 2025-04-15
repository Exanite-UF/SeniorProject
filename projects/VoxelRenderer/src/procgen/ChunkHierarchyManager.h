#pragma once

#include <format>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <src/utilities/Log.h>
#include <src/utilities/Singleton.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <iostream>

#include <src/procgen/data/TreeStructure.h>
#include <src/world/VoxelChunkData.h>

class StructureNode
{
public:
    TreeStructure structure;

    StructureNode(TreeStructure structure)
        : structure(structure)
    {
    }

    glm::ivec2 getMaxDistanceFromOrigin()
    {
        return structure.getMaxDistanceFromOrigin();
    }

    std::vector<glm::ivec3> overlappingChunks() {return std::vector<glm::ivec3>();};
};

class ChunkHierarchyManager : public Singleton<ChunkHierarchyManager>
{
    friend class Singleton;

private:
    // Each level contain a map from 2D voxel positions to lists of structures at that position
    // The higher levels span multiple of the lower levels, and they overlap
    // Then enables structures to cross chunk boundaries
    std::vector<std::unordered_map<glm::ivec2, std::vector<std::shared_ptr<StructureNode>>>> levels;
    std::vector<std::unordered_map<glm::ivec2, bool>> isGenerated;
    std::shared_mutex levelsMTX; // Lock the levels data structure

    glm::ivec3 chunkSize = glm::ivec3(0);

    void validateChunkSize() const;

    // Singleton needs default constructor
    explicit ChunkHierarchyManager()
        : ChunkHierarchyManager(0)
    {
    }

    explicit ChunkHierarchyManager(uint32_t levelCount)
    {
        for (int i = 0; i < levelCount; i++)
        {
            std::unordered_map<glm::ivec2, std::vector<std::shared_ptr<StructureNode>>> level;
            levels.push_back(level);

            std::unordered_map<glm::ivec2, bool> isLevelGenerated;
            isGenerated.push_back(isLevelGenerated);
        }
    }

public:
    std::mutex mutex;

    bool isChunkGenerated(glm::ivec2 chunkPosition, int level)
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

        //std::cout << (isChunkGenerated != isLevelGenerated.end()) << std::endl;

        if (isChunkGenerated != isLevelGenerated.end())
        {
            if(isChunkGenerated->second){
                //std::cout << temp.x << " " << temp.y << " Rejected" << std::endl;
            }else{
                //std::cout << temp.x << " " << temp.y << " Accepted" << std::endl;
            }
            return isChunkGenerated->second;
        }
        else
        {
            //std::cout << temp.x << " " << temp.y << " Accepted" << std::endl;
            return false;
        }
    }

    void setChunkGenerated(glm::ivec2 chunkPosition, int level, bool flag)
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

    // These require axis aligned voxel chunks

    /// @brief Add the seed point of a structure so that it can be queried for later
    /// @param chunkSize The size of a chunk in voxels
    /// @param structureOrigin //This is the world space position of the structure (in voxels)
    /// @param structure This is the actual structure
    void addStructure(glm::ivec3 structureOrigin, std::shared_ptr<StructureNode> structure);

    /// @brief Gets all the structure seeds whose structure could appear in the requested chunk
    /// @param chunkPosition A coordinate inside the chunk in question (in voxels)
    /// @param chunkSize The size of a chunk in voxels
    std::unordered_set<std::shared_ptr<StructureNode>> getStructuresForChunk(glm::ivec2 chunkPosition);

    void clear();

    void setChunkSize(glm::ivec3 size);

};
