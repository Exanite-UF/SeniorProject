#pragma once

#include <cstdint>
#include <vector>
#include <glm/vec3.hpp>

// 2D or 3D array of single values
// AKA Texture but that's already used. 
class TextureData 
{
public:
    glm::ivec3 size;
    std::vector<uint32_t> map;

    TextureData(glm::ivec3 size);
    uint32_t get(int x, int y, int z = 0);
    void set(uint32_t value, int x, int y, int z = 0);
};