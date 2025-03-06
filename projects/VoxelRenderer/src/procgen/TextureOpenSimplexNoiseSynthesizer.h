#pragma once

#include <src/procgen/TextureDataSynthesizer.h>

class TextureOpenSimplexNoiseSynthesizer : TextureDataSynthesizer
{
private:
    unsigned int seed;

public:
    TextureOpenSimplexNoiseSynthesizer(unsigned int seed);
    void generate(TextureData& textureData) override;
};