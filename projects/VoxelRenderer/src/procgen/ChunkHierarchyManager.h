#pragma once

#include <glm/glm.hpp>
#include <src/utilities/Singleton.h>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_set>

class StructureNode
{
};

class ChunkHierarchyManager : public Singleton<ChunkHierarchyManager>
{
private:
	std::vector<std::unordered_map<glm::ivec2, std::vector<std::shared_ptr<StructureNode>>>> levels;

public:
	std::mutex mutex;

	ChunkHierarchyManager(uint32_t levelCount)
	{
		for(int i = 0; i < levelCount; i++)
		{
			levels.emplace({});
		}
	}

	std::unordered_set<std::shared_ptr<StructureNode>> getStructuresFromChunk(glm::ivec2 chunkPosition)
	{
		std::unordered_set<std::shared_ptr<StructureNode>> allStructures;

		glm::vec2 chunkPositionLevel = chunkPosition;

		std::unordered_set<glm::vec2> quad = 
		{
			chunkPosition
		};

		for(int i = 0; i < levels.size(); i++)
		{
			auto& level = levels[i];

			for(auto& mappedChunkPosition : quad)
			{
				auto& levelStructures = level.find((glm::ivec2)mappedChunkPosition);

				if(levelStructures != level.end())
				{
					for(int j = 0; j < levelStructures->second.size(); j++)
					{
						allStructures.emplace(levelStructures->second[j]);
					}
				}
			}

			quad = 
			{
				{glm::floor(chunkPositionLevel.x / 2), glm::floor(chunkPositionLevel.y / 2)},
				{glm::ceil(chunkPositionLevel.x / 2), glm::floor(chunkPositionLevel.y / 2)},
				{glm::floor(chunkPositionLevel.x / 2), glm::ceil(chunkPositionLevel.y / 2)},
				{glm::ceil(chunkPositionLevel.x / 2), glm::ceil(chunkPositionLevel.y / 2)}
			};

			chunkPositionLevel = {glm::ceil(chunkPositionLevel.x / 2), glm::ceil(chunkPositionLevel.y / 2)};
		}

		return allStructures;
	}
};
