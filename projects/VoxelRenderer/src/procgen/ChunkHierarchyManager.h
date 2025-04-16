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

#include <src/world/VoxelChunkData.h>

class Structure
{
public:
    // This is for a bounding box check, so we can add and query for structures
    virtual glm::ivec2 getMaxDistanceFromOrigin() = 0;

    // This is so that we can reject the structure if it will attempt to generate in an already generated chunk
    virtual std::vector<glm::ivec3> overlappingChunks() = 0;

    // Gets the starting point from which the structure generates
    [[nodiscard]] virtual const glm::ivec3& getOriginVoxel() = 0;
    virtual void setOriginVoxel(glm::ivec3 origin) = 0;

    // TODO: somehow, reject structures that overlap with already generated structures.

    // Instantiates the structure into the voxel data
    virtual void generate(VoxelChunkData& chunkData) = 0;
};

class ChunkHierarchyManager : public Singleton<ChunkHierarchyManager>
{
    friend class Singleton;

private:
    // Each level contain a map from 2D voxel positions to lists of structures at that position
    // The higher levels span multiple of the lower levels, and they overlap
    // Then enables structures to cross chunk boundaries
    std::vector<std::unordered_map<glm::ivec2, std::vector<std::shared_ptr<Structure>>>> levels;
    std::vector<std::unordered_map<glm::ivec2, bool>> isGenerated;
    std::shared_mutex levelsMTX; // Lock the levels data structure

    glm::ivec3 chunkSize = glm::ivec3(0);

    void validateChunkSize() const;

public:
    std::mutex mutex;

    bool isChunkGenerated(glm::ivec2 chunkPosition, int level);
    void setChunkGenerated(glm::ivec2 chunkPosition, int level, bool flag);

    // These require axis aligned voxel chunks

    /// @brief Add the seed point of a structure so that it can be queried for later
    /// @param chunkSize The size of a chunk in voxels
    /// @param structureOrigin //This is the world space position of the structure (in voxels)
    /// @param structure This is the actual structure
    void addStructure(glm::ivec3 structureOrigin, std::shared_ptr<Structure> structure);

    /// @brief Gets all the structure seeds whose structure could appear in the requested chunk
    /// @param chunkPosition A coordinate inside the chunk in question (in voxels)
    /// @param chunkSize The size of a chunk in voxels
    std::unordered_set<std::shared_ptr<Structure>> getStructuresForChunk(glm::ivec2 chunkPosition);

    void clear();

    void setChunkSize(glm::ivec3 size);
};
