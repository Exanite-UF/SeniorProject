#include <src/procgen/synthesizers/TextureOctaveNoiseSynthesizer.h>

#include <PerlinNoise/PerlinNoise.hpp>
#include <imgui/imgui.h>

TextureOctaveNoiseSynthesizer::TextureOctaveNoiseSynthesizer(int seed, int octaves, float persistence)
{
    this->seed = seed;
    this->octaves = octaves;
    this->persistence = persistence;
}

void TextureOctaveNoiseSynthesizer::generate(std::shared_ptr<TextureData>& textureData)
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
    // To Fix the Long title issue for headers
    std::string headerText = "Octave Noise Synthesizer";

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

        ImGui::Text("Octaves");
        ImGui::Indent(indentSize);
        ImGui::PushID("octaves");
        ImGui::SliderInt("", &octaves, 1, 100);
        ImGui::PopID();
        ImGui::Unindent(indentSize);

        ImGui::Text("Persistence");
        ImGui::Indent(indentSize);
        ImGui::PushID("persistence");
        ImGui::SliderFloat("", &persistence, 0, 1);
        ImGui::PopID();
        ImGui::Unindent(indentSize);
    }
}

std::function<float(float)> TextureOctaveNoiseSynthesizer::mapperTo01()
{
    return [](float sample)
    {
        return sample;
    };
}
