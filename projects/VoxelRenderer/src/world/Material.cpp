#include "Material.h"

Material::Material() = default;

Material::Material(uint16_t index, const std::string& key, const std::string& name)
{
    this->index = index;
    this->key = key;
    this->name = name;
}

int16_t Material::getIndex() const
{
    return index;
}

const std::string& Material::getKey() const
{
    return key;
}
