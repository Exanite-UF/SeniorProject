#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <vector>

// 2D or 3D array of single values
// AKA Texture but that's already used.
class TextureData
{
private:
    glm::ivec3 size;
    std::vector<float> map;

public:
    TextureData(glm::ivec3 inSize);
    float get(int x, int y, int z = 0);
    void set(float value, int x, int y, int z = 0);

    const glm::ivec3& getSize();
};
