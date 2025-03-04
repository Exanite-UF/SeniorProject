#pragma once

#include <src/procgen/TextureDataSynthesizer.h>

class TextureOctaveNoiseSynthesizer : TextureDataSynthesizer
{
private:
    float seed;
    int octaves = 3;
    float persistence = 0.5;

public:
    TextureOctaveNoiseSynthesizer(float seed, int octaves, float persistence);
    void generate(TextureData& textureData) override;
};