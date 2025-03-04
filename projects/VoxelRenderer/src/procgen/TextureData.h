#pragma once

#include <cstdint>
#include <vector>
#include <glm/vec3.hpp>

// 2D or 3D array of single values
// AKA Texture but that's already used. 
class TextureData 
{
private:
    glm::ivec3 size;
    std::vector<uint32_t> map;
    
public: 
    TextureData(glm::ivec3 inSize);
    uint32_t get(int x, int y, int z = 0);
    void set(uint32_t value, int x, int y, int z = 0);

    const glm::ivec3& getSize();
};