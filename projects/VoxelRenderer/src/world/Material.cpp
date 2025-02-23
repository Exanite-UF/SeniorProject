#include "Material.h"

Material::Material() = default;

Material::Material(uint16_t index, std::string id)
{
    this->index = index;
    this->id = id;
}
