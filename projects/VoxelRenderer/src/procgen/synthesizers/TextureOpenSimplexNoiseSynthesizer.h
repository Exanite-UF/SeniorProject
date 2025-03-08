#pragma once

#include <src/procgen/synthesizers/TextureDataSynthesizer.h>

class TextureOpenSimplexNoiseSynthesizer : public TextureDataSynthesizer
{
private:
    int seed;

public:
    TextureOpenSimplexNoiseSynthesizer(int seed);
    void generate(std::shared_ptr<TextureData>& textureData) override;
    void showDebugMenu() override;
    std::function<float(float)> mapperTo01() override; 
};
