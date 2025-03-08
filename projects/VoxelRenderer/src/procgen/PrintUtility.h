#pragma once

#include <src/procgen/data/TextureData.h>
#include <functional>

class PrintUtility
{
private:


public:
    static void printTexture(std::shared_ptr<TextureData>& textureData, std::function<float(float)> mapTo01, const std::string& filename);
};