#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <vector>

// 2D or 3D array of single values
// AKA Texture but that's already used.
template <typename T> 
class FlatArrayData
{
private:
    glm::ivec3 size;
    std::vector<T> map;

public:
    FlatArrayData(glm::ivec3 inSize);
    bool copyList(std::initializer_list<T> list);
    T get(int x, int y, int z = 0);
    void set(T value, int x, int y, int z = 0);

    const glm::ivec3& getSize();
};

template <typename T>
FlatArrayData<T>::FlatArrayData(glm::ivec3 inSize)
{
    this->size = inSize;
    this->map.resize(size.x * size.y * size.z, 0);
}

template <typename T>
bool FlatArrayData<T>::copyList(std::initializer_list<T> list)
{
    if(list.size() != size.x * size.y * size.z)
    {
        return false;
    }

    map = list;
    return true;
}

template <typename T>
T FlatArrayData<T>::get(int x, int y, int z)
{
    return this->map[x + y * size.x + z * size.y * size.x];
}

template <typename T>
void FlatArrayData<T>::set(T value, int x, int y, int z)
{
    this->map[x + y * size.x + z * size.y * size.x] = value;
}

template <typename T>
const glm::ivec3& FlatArrayData<T>::getSize()
{
    return size;
}

