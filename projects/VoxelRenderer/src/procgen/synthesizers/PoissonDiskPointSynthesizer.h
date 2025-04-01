#pragma once

#include <src/procgen/synthesizers/PointSynthesizer.h>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

class PoissonDiskPointSynthesizer : public PointSynthesizer
{
private:
	glm::vec2 rangeX = {0, 1};
	glm::vec2 rangeY = {0, 1};

public:
	PoissonDiskPointSynthesizer(int seed) : PointSynthesizer(seed) {}
	void generatePoints(std::vector<glm::vec3>& inPoints, uint32_t numPoints) override;
	void rescalePointsToChunkSize(std::vector<glm::vec3>& inPoints, VoxelChunkData& chunkData) override;
	void showDebugMenu() override;
};