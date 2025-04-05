#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <src/utilities/ImGui.h>

class ImGuiUtility
{
public:
    static ImVec2 toImGui(const glm::vec2& vec);
    static glm::vec2 toGlm(const ImVec2& vec);

    static ImVec4 toImGui(const glm::vec4& vec);
    static glm::vec4 toGlm(const ImVec4& vec);
};
