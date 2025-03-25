#include "FlatArrayData.h"


template <typename T>
TextureData<T>::TextureData(glm::ivec3 inSize)
{
    this->size = inSize;
    this->map.resize(size.x * size.y * size.z, 0);
}

template <typename T>
T TextureData<T>::get(int x, int y, int z)
{
    return this->map[x + y * size.x + z * size.y * size.x];
}

template <typename T>
void TextureData<T>::set(T value, int x, int y, int z)
{
    this->map[x + y * size.x + z * size.y * size.x] = value;
}

template <typename T>
const glm::ivec3& TextureData<T>::getSize()
{
    return size;
}
