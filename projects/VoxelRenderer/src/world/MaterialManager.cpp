#include "MaterialManager.h"

#include <array>
#include <span>

MaterialManager::MaterialManager()
{
}

MaterialManager::~MaterialManager()
{
}

void MaterialManager::writeToGpu()
{
    materialMapBuffer.readFrom(materialMap);
    materialDataBuffer.readFrom(materialData);
}

GraphicsBuffer<uint32_t>& MaterialManager::getMaterialMapBuffer()
{
    return materialMapBuffer;
}

GraphicsBuffer<MaterialData>& MaterialManager::getMaterialDataBuffer()
{
    return materialDataBuffer;
}
