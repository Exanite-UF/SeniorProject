#include "TextureDataSynthesizer.h"

#include <PerlinNoise/PerlinNoise.hpp>

TextureDataSynthesizer::TextureDataSynthesizer(float seed)
{
    this->seed = seed;
}

void TextureDataSynthesizer::generateOctaveNoise(TextureData& textureData, int32_t octaves, float persistence)
{
    siv::BasicPerlinNoise<float> perlinNoise(seed);

    for (int x = 0; x < textureData.size.x; ++x)
    {
        for (int y = 0; y < textureData.size.y; ++y)
        {
            float noise = perlinNoise.octave2D_01(((float)x) / textureData.size.x, ((float)y) / textureData.size.y, octaves, persistence);
            textureData.set(noise, x, y);
        }
    }
}