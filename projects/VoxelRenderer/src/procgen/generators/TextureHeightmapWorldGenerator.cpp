#include <PerlinNoise/PerlinNoise.hpp>
#include <imgui/imgui.h>
#include <iostream>
#include <memory>
#include <src/procgen/PrintUtility.h>
#include <src/procgen/generators/TextureHeightmapWorldGenerator.h>
#include <src/world/VoxelChunkData.h>

void TextureHeightmapWorldGenerator::generateData(VoxelChunkData& data)
{
    glm::ivec3 size = { data.getSize().x, data.getSize().y, 1 };
    this->textureData = std::make_shared<TextureData>(size);
    textureDataSynthesizer->generate(textureData);

    for (int x = 0; x < data.getSize().x; ++x)
    {
        for (int y = 0; y < data.getSize().y; ++y)
        {
            float noise = textureData->get(x, y);
            int offset = (int)(baseHeight + (noise * data.getSize().z));
            int height = glm::min(data.getSize().z, offset);

            for (int z = 0; z < height; ++z)
            {
                data.setVoxelOccupancy({ x, y, z }, true);
            }
        }
    }
}

TextureHeightmapWorldGenerator::TextureHeightmapWorldGenerator(const glm::ivec3& chunkSize, const std::shared_ptr<TextureDataSynthesizer>& textureDataSynthesizer)
{
    this->chunkSize = chunkSize;
    this->textureDataSynthesizer = textureDataSynthesizer;
}

void TextureHeightmapWorldGenerator::showDebugMenu()
{
    // To Fix the Long title issue for headers
    std::string headerText = "Texture-Heightmap World Generator";

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

    // TODO: Testing. Once finalized, add to existing Imgui fields.
    ImGui::PushID("TextureHeightmapWorldGenerator");
    {
        ImGui::PushTextWrapPos(ImGui::GetWindowContentRegionMax().x);
        float indentSize = ImGui::GetWindowContentRegionMax().x / 16.0f;
        if (ImGui::CollapsingHeader(headerText.c_str()))
        {
            ImGui::Text("Base Height");
            ImGui::Indent(indentSize);
            ImGui::PushID("BaseHeight");
            ImGui::SliderFloat("", &baseHeight, 0, chunkSize.z);
            ImGui::PopID();
            ImGui::Unindent(indentSize);
            textureDataSynthesizer->showDebugMenu();
            if (ImGui::Button("Print Texture"))
            {
                PrintUtility::printTexture(textureData, textureDataSynthesizer->mapperTo01(), "output_texture.ppm");
            }
        }
        ImGui::PopTextWrapPos();
    }
    ImGui::PopID();
}
