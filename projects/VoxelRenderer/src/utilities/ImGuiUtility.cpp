#include <src/utilities/ImGuiUtility.h>

ImVec2 ImGuiUtility::toImVec2(const glm::vec2& vec)
{
    return ImVec2(vec.x, vec.y);
}

glm::vec2 ImGuiUtility::toGlmVec2(const ImVec2& vec)
{
    return glm::vec2(vec.x, vec.y);
}

ImVec4 ImGuiUtility::toImVec4(const glm::vec4& vec)
{
    return ImVec4(vec.x, vec.y, vec.z, vec.w);
}

glm::vec4 ImGuiUtility::toGlmVec4(const ImVec4& vec)
{
    return glm::vec4(vec.x, vec.y, vec.z, vec.w);
}
