#include "Material.h"

Material::Material() = default;

Material::Material(uint16_t index, std::string id, std::string name)
{
    this->index = index;
    this->id = id;
    this->name = name;
}

int16_t Material::getIndex() const
{
    return index;
}

const std::string& Material::getId() const
{
    return id;
}
