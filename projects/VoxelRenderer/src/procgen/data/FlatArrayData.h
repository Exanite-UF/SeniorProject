#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <vector>

// 2D or 3D array of single values
// AKA Texture but that's already used.
template <typename T> 
class TextureData
{
private:
    glm::ivec3 size;
    std::vector<T> map;

public:
    TextureData(glm::ivec3 inSize);
    T get(int x, int y, int z = 0);
    void set(T value, int x, int y, int z = 0);

    const glm::ivec3& getSize();
};
