#include "TextureData.h"


TextureData::TextureData(glm::ivec3 size){
    this->size = size;
}

uint32_t TextureData::get(int x, int y, int z){
    return map[x + y * size.x + z * size.y * size.x];
}

void TextureData::set(uint32_t value, int x, int y, int z){
    map[x + y * size.x + z * size.y * size.x] = value;
}