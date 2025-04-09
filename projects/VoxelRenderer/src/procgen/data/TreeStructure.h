#pragma once

#include <src/world/VoxelChunkData.h>
#include <tracy/Tracy.hpp>

class TreeStructure
{
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

	void generateRectangle(VoxelChunkData& chunkData, glm::vec3 originVoxel, std::shared_ptr<Material>& material, int widthX, int widthY, int height);
	void generateAbsPyramid(VoxelChunkData& chunkData,  glm::vec3 originVoxel, std::shared_ptr<Material>& material, int widthX, int widthY, int extentAboveZ, int extentBelowZ, float probabilityToFill);

public:
	// Ugly. Ideally pass in via single data object, but materials are references. 
	TreeStructure(
		std::shared_ptr<Material> logMaterial,
		std::shared_ptr<Material> leafMaterial,
		int treeHeightVoxels,
		int treeWidthVoxels,
		int leafWidthX,
		int leafWidthY,
		int leafExtentBelowZ,
		int leafExtentAboveZ,
		float leafProbabilityToFill
	);
	void generate(VoxelChunkData& chunkData, glm::vec3 originVoxel);
};

TreeStructure::TreeStructure(
	std::shared_ptr<Material> logMaterial,
	std::shared_ptr<Material> leafMaterial,
	int treeHeightVoxels,
	int treeWidthVoxels,
	int leafWidthX,
	int leafWidthY,
	int leafExtentBelowZ,
	int leafExtentAboveZ,
	float leafProbabilityToFill
) : logMaterial(logMaterial), leafMaterial(leafMaterial)
{
	this->treeHeightVoxels = treeHeightVoxels;
	this->treeWidthVoxels = treeWidthVoxels;
	this->leafWidthX = leafWidthX;
	this->leafWidthY = leafWidthY;
	this->leafExtentBelowZ = leafExtentBelowZ;
	this->leafExtentAboveZ = leafExtentAboveZ;
	this->leafProbabilityToFill = leafProbabilityToFill;
}

void TreeStructure::generate(VoxelChunkData& chunkData, glm::vec3 originVoxel)
{
	ZoneScoped;

    // Tree Trunk
    generateRectangle(
		chunkData,
		originVoxel, 
		logMaterial,
		treeWidthVoxels, 
		treeWidthVoxels, 
		treeHeightVoxels
	);

    glm::vec3 originOffset = { 0, 0, treeHeightVoxels + 1 };
    originVoxel += originOffset;

    generateAbsPyramid(
		chunkData, 
		originVoxel, 
		leafMaterial, 
		leafWidthX, 
		leafWidthY, 
		leafExtentAboveZ, 
		leafExtentBelowZ, 
		leafProbabilityToFill
	);
}

void TreeStructure::generateRectangle(VoxelChunkData& chunkData, glm::vec3 originVoxel, std::shared_ptr<Material>& material, int widthX, int widthY, int height)
{
    int widthXOffset = widthX / 2;
    int widthYOffset = widthY / 2;

    for (int localX = 0; localX <= widthX; ++localX)
    {
        for (int localY = 0; localY <= widthY; ++localY)
        {
            for (int localZ = 0; localZ <= height; ++localZ)
            {
                glm::vec3 localVoxel = { originVoxel.x + localX - widthXOffset, originVoxel.y + localY - widthYOffset, originVoxel.z + localZ };

				if(!chunkData.isValidPosition(localVoxel)) 
				{
					continue;
				}

                chunkData.setVoxelOccupancy(localVoxel, true);
                chunkData.setVoxelMaterial(localVoxel, material);
            }
        }
    }
}

void TreeStructure::generateAbsPyramid(VoxelChunkData& chunkData, glm::vec3 originVoxel, std::shared_ptr<Material>& material, int widthX, int widthY, int extentAboveZ, int extentBelowZ, float probabilityToFill)
{
    int leafWidthRadiusX = widthX / 2;
    int leafWidthRadiusY = widthY / 2;

    // Setup tree function
    int heightZ = extentAboveZ;
    float heightToWidthXRatio = ((float)heightZ) / widthX;
    float heightToWidthYRatio = ((float)heightZ) / widthY;

    for (int localX = -leafWidthRadiusX; localX <= leafWidthRadiusX; ++localX)
    {
        for (int localY = -leafWidthRadiusY; localY <= leafWidthRadiusY; ++localY)
        {
            for (int localZ = -extentBelowZ; localZ <= extentAboveZ; ++localZ)
            {
                glm::vec3 localVoxel = { originVoxel.x + localX, originVoxel.y + localY, originVoxel.z + localZ };

                // Fall through
				if(!chunkData.isValidPosition(localVoxel)) 
				{
					continue;
				}

                int treeFunctionSample = heightZ - heightToWidthXRatio * abs(localX) - heightToWidthYRatio * abs(localY);
                // Simple random function. Probably better to clump and also add so it looks more organic.
                bool randomSample = ((float)rand() / RAND_MAX) >= probabilityToFill;

                if (localZ <= treeFunctionSample && randomSample)
                {
                    if (chunkData.getVoxelMaterial(localVoxel) != material)
                    {
                        chunkData.setVoxelOccupancy(localVoxel, true);
                        chunkData.setVoxelMaterial(localVoxel, material);
                    }
                }
            }
        }
    }
}