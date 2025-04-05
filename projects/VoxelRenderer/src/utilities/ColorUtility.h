#pragma once

#include <glm/common.hpp>
#include <string>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

class ColorUtility
{
public:
    // Converts sRGB [0, 1] to linear [0, 1]
    static float srgbToLinear(float value);

    // Converts linear [0, 1] to sRGB [0, 1]
    static float linearToSrgb(float value);

    static glm::vec3 srgbToLinear(glm::vec3 value);
    static glm::vec3 linearToSrgb(glm::vec3 value);

    static glm::vec4 srgbToLinear(glm::vec4 value);
    static glm::vec4 linearToSrgb(glm::vec4 value);

    // Converts sRGB [0, 255] to linear [0, 1]
    static glm::vec4 srgbToLinear(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

    // Converts an HTML/hexadecimal color code to linear [0, 1]
    // The alpha component can be excluded
    static glm::vec4 htmlToLinear(const std::string& value);
    static glm::vec4 htmlToSrgb(std::string value);

    // Color is expected in hexadecimal form, encoded as an integer. No alpha channel.
    // Eg: int color = 0xffffff
    static std::string hexColorToAnsi(int color);

    static std::string ansiForeground(int color);
    static std::string ansiBackground(int color);

    static std::string ansiReset();
};
