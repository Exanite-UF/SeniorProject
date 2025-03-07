#include "TextureData.h"

TextureData::TextureData(glm::ivec3 inSize)
{
    this->size = inSize;
    this->map.resize(size.x * size.y * size.z, 0);
}

float TextureData::get(int x, int y, int z)
{
    return this->map[x + y * size.x + z * size.y * size.x];
}

void TextureData::set(float value, int x, int y, int z)
{
    this->map[x + y * size.x + z * size.y * size.x] = value;
}

const glm::ivec3& TextureData::getSize()
{
    return size;
}
