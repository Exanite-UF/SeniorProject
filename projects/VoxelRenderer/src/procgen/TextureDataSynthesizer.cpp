#include "TextureDataSynthesizer.h"

#include <PerlinNoise/PerlinNoise.hpp>

TextureDataSynthesizer::TextureDataSynthesizer(float seed)
{
    this->seed = seed;
}

void TextureDataSynthesizer::generateOctaveNoise(TextureData& textureData, int32_t octaves, float persistence)
{
    siv::BasicPerlinNoise<float> perlinNoise(seed);

    for (int y = 0; y < textureData.getSize().y; y++)
    { 
        for (int x = 0; x < textureData.getSize().x; x++)
        {
            float noise = perlinNoise.octave2D_01(((float)x) / textureData.getSize().x, ((float)y) / textureData.getSize().y, octaves, persistence);
            textureData.set(noise, x, y);
        }
    }
}