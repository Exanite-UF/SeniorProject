#include "TextureOctaveNoiseSynthesizer.h"

#include <PerlinNoise/PerlinNoise.hpp>

TextureOctaveNoiseSynthesizer:: TextureOctaveNoiseSynthesizer(float seed, int octaves, float persistence) 
{
    this->seed = seed;
    this->octaves = octaves;
    this->persistence = persistence;
}

void TextureOctaveNoiseSynthesizer::generate(TextureData& textureData)
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
