#pragma once

#include <src/procgen/synthesizers/TextureDataSynthesizer.h>

class TextureOpenSimplexNoiseSynthesizer : public TextureDataSynthesizer
{
private:
    int seed;

public:
    TextureOpenSimplexNoiseSynthesizer(int seed);
    void generate(TextureData& textureData) override;
    void showDebugMenu() override;
};
