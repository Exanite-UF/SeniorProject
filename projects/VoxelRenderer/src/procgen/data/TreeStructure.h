#pragma once

#include <memory>
#include <src/world/VoxelChunkData.h>

#include <src/procgen/ChunkHierarchyManager.h>

class TreeStructure : public Structure
{
public:
    glm::ivec3 originVoxel;

private:
    std::shared_ptr<Material> logMaterial;
    std::shared_ptr<Material> leafMaterial;
    int treeHeightVoxels;
    int treeWidthVoxels;
    int leafWidthX;
    int leafWidthY;
    int leafExtentBelowZ;
    int leafExtentAboveZ;
    float leafProbabilityToFill;

    glm::ivec2 maxDistanceFromOrigin;

    // TODO: Fix vec3 to ivec3
    void generateRectangle(VoxelChunkData& chunkData, glm::vec3 originVoxel, std::shared_ptr<Material>& material, int widthX, int widthY, int height);
    void generateAbsPyramid(VoxelChunkData& chunkData, glm::vec3 originVoxel, std::shared_ptr<Material>& material, int widthX, int widthY, int extentAboveZ, int extentBelowZ, float probabilityToFill);

public:
    TreeStructure(
        glm::ivec3 originVoxel,
        std::shared_ptr<Material> logMaterial,
        std::shared_ptr<Material> leafMaterial,
        int treeHeightVoxels,
        int treeWidthVoxels,
        int leafWidthX,
        int leafWidthY,
        int leafExtentBelowZ,
        int leafExtentAboveZ,
        float leafProbabilityToFill);

    virtual void generate(VoxelChunkData& chunkData) override;
    [[nodiscard]] virtual const glm::ivec3& getOriginVoxel() override;
    virtual glm::ivec2 getMaxDistanceFromOrigin() override;

    virtual void setOriginVoxel(glm::ivec3 origin) override;

    virtual std::vector<glm::ivec3> overlappingChunks() override;
};
