#pragma once

#include <src/world/VoxelChunkData.h>

// Given some parameters, create a set of points.
// Can create random points, or distributed.
// Useful for creating polygons or hulls, possible cyclic paths.
class PointSynthesizer : public NonCopyable
{
protected:
	int seed;
public:
	PointSynthesizer(int seed) { this->seed = seed; }

	virtual void generatePoints(std::vector<glm::vec3>& inPoints, uint32_t numPoints) = 0;
	virtual void rescalePointsToChunkSize(std::vector<glm::vec3>& inPoints, VoxelChunkData& chunkData) = 0;
	virtual void showDebugMenu() = 0;
	void setSeed(int seed) { this->seed = seed; }
};
