#include <src/procgen/synthesizers/TextureOpenSimplexNoiseSynthesizer.h>

#include <FastNoiseLite/FastNoiseLite.h>
#include <imgui/imgui.h>
#include <string>

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
            float noise = (simplexNoise.GetNoise((float)x, (float)y) + 1) * 0.5f;
            textureData->set(noise, x, y);
        }
    }
}

void TextureOpenSimplexNoiseSynthesizer::showDebugMenu()
{
    // To Fix the Long title issue for headers
    std::string headerText = "Open Simplex Noise Synthesizer";

    float availableWidth = ImGui::GetContentRegionAvail().x * 0.9f;
    float textWidth = ImGui::CalcTextSize(headerText.c_str()).x;

    if (textWidth > availableWidth)
    {
        std::string ellipsis = "...";
        float ellipsisWidth = ImGui::CalcTextSize(ellipsis.c_str()).x;

        while (ImGui::CalcTextSize((headerText + ellipsis).c_str()).x > (availableWidth) && headerText.length() > 1)
        {
            headerText.pop_back();
        }

        headerText += ellipsis;
    }

    float indentSize = ImGui::GetWindowContentRegionMax().x / 16.0f;
    if (ImGui::CollapsingHeader(headerText.c_str()))
    {
        ImGui::Text("Seed");
        ImGui::Indent(indentSize);
        ImGui::PushID("seedOctave");
        ImGui::SliderInt("", &seed, 0, 100);
        ImGui::PopID();
        ImGui::Unindent(indentSize);
    }
}

std::function<float(float)> TextureOpenSimplexNoiseSynthesizer::mapperTo01()
{
    return [](float sample)
    {
        return sample;
    };
}
