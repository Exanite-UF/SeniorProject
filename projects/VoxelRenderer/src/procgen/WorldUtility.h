#pragma once

#include <src/utilities/Log.h>
#include <src/world/MaterialManager.h>

class WorldUtility
{
public:
    static bool tryGetMaterial(std::string materialKey, MaterialManager& materialManager, std::shared_ptr<Material>& outMaterial)
    {
        if (!materialManager.tryGetMaterialByKey(materialKey, outMaterial))
        {
            outMaterial = materialManager.getMaterialByIndex(0);
            Log::information("Failed to find material with id '" + materialKey + "'. Using default material '" + outMaterial->getKey() + "' instead.");
            return false;
        }

        return true;
    }
};
