#include <src/procgen/synthesizers/TextureOpenSimplexNoiseSynthesizer.h>

#include <FastNoiseLite/FastNoiseLite.h>
#include <imgui/imgui.h>

TextureOpenSimplexNoiseSynthesizer::TextureOpenSimplexNoiseSynthesizer(int seed)
{
    this->seed = seed;
}

void TextureOpenSimplexNoiseSynthesizer::generate(std::shared_ptr<TextureData>& textureData)
{
    FastNoiseLite simplexNoise;
    simplexNoise.SetSeed(seed);
    simplexNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    for (int y = 0; y < textureData->getSize().y; y++)
    {
        for (int x = 0; x < textureData->getSize().x; x++)
        {
            float noise = simplexNoise.GetNoise((float)x, (float)y);
            textureData->set(noise, x, y);
        }
    }
}

void TextureOpenSimplexNoiseSynthesizer::showDebugMenu()
{
    if (ImGui::CollapsingHeader("Open Simplex Noise Synthesizer"))
    {
        ImGui::SliderInt("Seed", &seed, 0, 100);
    }
}

std::function<float(float)> TextureOpenSimplexNoiseSynthesizer::mapperTo01() 
{
    return [](float sample) { return (sample + 1)/2; };
}
