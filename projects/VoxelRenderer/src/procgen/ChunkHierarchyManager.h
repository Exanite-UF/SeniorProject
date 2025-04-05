#pragma once

#include <src/utilities/Singleton.h>

class ChunkHierarchyManager : public Singleton<ChunkHierarchyManager>
{
    // Chunk0 position: [3]
    // Chunk1 position: 3   / 2   -> 1.5  | floor(1.5), ceil(1.5) -> 1, [2]
    // Chunk2 position: 2   / 2   -> 1    | floor(1)  , ceil(1)   -> [1]

    // Chunk0 position: [5]
    // Chunk1 position: 5   / 2   -> 2.5  | floor(2.5), ceil(2.5) -> 2, [3]
    // Chunk2 position: 3   / 2   -> 1.5  | floor(1.5), ceil(1.5) -> 1, 2

    // std::unordered_map<int, std::vector<Structure>> chunk0;
    // std::unordered_map<int, std::vector<Structure>> chunk1;
    // std::unordered_map<int, std::vector<Structure>> chunk2;

    // std::vector<std::unordered_map<int, std::vector<Structure>>> chunks;
    // std::array<std::unordered_map<int, std::vector<Structure>>, 3> chunks;
};
