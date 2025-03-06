#pragma once

#include <src/procgen/TextureDataSynthesizer.h>

class TextureOctaveNoiseSynthesizer : public TextureDataSynthesizer
{
private:
    unsigned int seed;
    int octaves = 3;
    float persistence = 0.5;

public:
    TextureOctaveNoiseSynthesizer(unsigned int seed, int octaves, float persistence);
    void generate(TextureData& textureData) override;
    void showDebugMenu() override;
};