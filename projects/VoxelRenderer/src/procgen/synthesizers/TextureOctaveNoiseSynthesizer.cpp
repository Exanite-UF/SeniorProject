#include <src/procgen/synthesizers/TextureOctaveNoiseSynthesizer.h>

#include <PerlinNoise/PerlinNoise.hpp>
#include <imgui/imgui.h>

TextureOctaveNoiseSynthesizer::TextureOctaveNoiseSynthesizer(int seed, int octaves, float persistence)
{
    this->seed = seed;
    this->octaves = octaves;
    this->persistence = persistence;
}

void TextureOctaveNoiseSynthesizer::generate(std::shared_ptr<TextureDataA>& textureData)
{
    siv::BasicPerlinNoise<float> perlinNoise(seed);

    for (int y = 0; y < textureData->getSize().y; y++)
    {
        for (int x = 0; x < textureData->getSize().x; x++)
        {
            float noise = perlinNoise.octave2D_01(((float)x) / textureData->getSize().x, ((float)y) / textureData->getSize().y, octaves, persistence);
            textureData->set(noise, x, y);
        }
    }
}

void TextureOctaveNoiseSynthesizer::showDebugMenu()
{
    if (ImGui::CollapsingHeader("Octave Noise Synthesizer"))
    {
        ImGui::SliderInt("Seed", &seed, 0, 100);
        ImGui::SliderInt("Octaves", &octaves, 1, 100);
        ImGui::SliderFloat("Persistence", &persistence, 0, 1);
    }
}

std::function<float(float)> TextureOctaveNoiseSynthesizer::mapperTo01()
{
    return [](float sample)
    {
        return sample;
    };
}
