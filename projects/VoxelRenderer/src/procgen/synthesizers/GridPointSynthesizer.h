#pragma once

#include <src/procgen/synthesizers/PointSynthesizer.h>

class GridPointSynthesizer : public PointSynthesizer
{
private:
	glm::vec2 paddingXY = { 0, 0.9 };
	glm::vec2 rangeX = { 0, 1 };
	glm::vec2 rangeY = { 0, 1 };

public:
	GridPointSynthesizer(int seed)
	: PointSynthesizer(seed){}
	void generatePoints(std::vector<glm::vec3>& inPoints, uint32_t numPoints) override;
	void rescalePointsToChunkSize(std::vector<glm::vec3>& inPoints, VoxelChunkData& chunkData) override;
	void showDebugMenu() override;
};