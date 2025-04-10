#pragma once

#include <functional>
#include <memory>
#include <src/procgen/data/TextureData.h>
#include <string>

class PrintUtility
{
private:
public:
    static void printTexture(std::shared_ptr<TextureDataA>& textureData, std::function<float(float)> mapTo01, const std::string& filename);
};
