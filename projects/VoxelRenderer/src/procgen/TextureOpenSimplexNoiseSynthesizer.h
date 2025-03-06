#pragma once

#include <src/procgen/TextureDataSynthesizer.h>

class TextureOpenSimplexNoiseSynthesizer : public TextureDataSynthesizer
{
private:
    unsigned int seed;

public:
    TextureOpenSimplexNoiseSynthesizer(unsigned int seed);
    void generate(TextureData& textureData) override;
    void showDebugMenu() override;
};