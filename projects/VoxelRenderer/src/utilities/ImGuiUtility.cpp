#include <src/utilities/ImGuiUtility.h>

ImVec2 ImGuiUtility::toImGui(const glm::vec2& vec)
{
    return ImVec2(vec.x, vec.y);
}

glm::vec2 ImGuiUtility::toGlm(const ImVec2& vec)
{
    return glm::vec2(vec.x, vec.y);
}

ImVec4 ImGuiUtility::toImGui(const glm::vec4& vec)
{
    return ImVec4(vec.x, vec.y, vec.z, vec.w);
}

glm::vec4 ImGuiUtility::toGlm(const ImVec4& vec)
{
    return glm::vec4(vec.x, vec.y, vec.z, vec.w);
}

std::string ImGuiUtility::getCondensedHeader(std::string headerText)
{
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

    return headerText;
}
