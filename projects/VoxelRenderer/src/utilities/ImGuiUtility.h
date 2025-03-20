#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <src/utilities/ImGui.h>

class ImGuiUtility
{
public:
    static ImVec2 toImVec2(const glm::vec2& vec);
    static glm::vec2 toGlmVec2(const ImVec2& vec);

    static ImVec4 toImVec4(const glm::vec4& vec);
    static glm::vec4 toGlmVec4(const ImVec4& vec);
};
