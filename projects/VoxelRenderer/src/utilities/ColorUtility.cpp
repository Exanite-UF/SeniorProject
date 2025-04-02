#include "ColorUtility.h"

#include <format>

#include "Assert.h"

float ColorUtility::srgbToLinear(float value)
{
    if (value <= 0.04045f)
    {
        return value / 12.92f;
    }

    return pow((value + 0.055f) / 1.055f, 2.4f);
}

float ColorUtility::linearToSrgb(float value)
{
    if (value <= 0.0031308f)
    {
        return value * 12.92f;
    }

    return pow(value, 1 / 2.4f) * 1.055f - 0.055f;
}

glm::vec3 ColorUtility::srgbToLinear(glm::vec3 value)
{
    return glm::vec3(srgbToLinear(value.x), srgbToLinear(value.y), srgbToLinear(value.z));
}

glm::vec3 ColorUtility::linearToSrgb(glm::vec3 value)
{
    return glm::vec3(linearToSrgb(value.x), linearToSrgb(value.y), linearToSrgb(value.z));
}

glm::vec4 ColorUtility::srgbToLinear(glm::vec4 value)
{
    return glm::vec4(srgbToLinear(value.x), srgbToLinear(value.y), srgbToLinear(value.z), value.w);
}

glm::vec4 ColorUtility::linearToSrgb(glm::vec4 value)
{
    return glm::vec4(linearToSrgb(value.x), linearToSrgb(value.y), linearToSrgb(value.z), value.w);
}

glm::vec4 ColorUtility::srgbToLinear(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return glm::vec4(linearToSrgb(r / 255.0f), linearToSrgb(g / 255.0f), linearToSrgb(b / 255.0f), a / 255.0f);
}

glm::vec4 ColorUtility::htmlToLinear(const std::string& value)
{
    return srgbToLinear(htmlToSrgb(value));
}

glm::vec4 ColorUtility::htmlToSrgb(std::string value)
{
    Assert::isTrue(value.size() > 0, "Color code is empty");

    // Remove leading '#'
    if (value.at(0) == '#')
    {
        value = value.substr(1);
    }

    // Expand colors like #fff to their full length
    if (value.size() == 3 || value.size() == 4)
    {
        std::string temp;
        for (int i = 0; i < value.size(); ++i)
        {
            temp += value[i];
            temp += value[i];
        }
        value = temp;
    }

    // Add default alpha component to color code
    if (value.size() == 6)
    {
        value += "ff";
    }

    Assert::isTrue(value.size() == 8, "Invalid length for input color code");

    // Parse individual components as hexadecimal [0, 255] range
    glm::vec4 result;
    for (int i = 0; i < 4; ++i)
    {
        result[i] = std::stoi(value.substr(i * 2, 2), nullptr, 16);
    }

    // Convert result to [0, 1] range
    result /= 255;

    return result;
}

std::string ColorUtility::hexColorToAnsi(int color)
{
    return std::format("{};{};{}", (color >> 16) & 0b11111111, (color >> 8) & 0b11111111, (color >> 0) & 0b11111111);
}

std::string ColorUtility::ansiForeground(int color)
{
    return std::format("\u001B[38;2;{}m", hexColorToAnsi(color));
}

std::string ColorUtility::ansiBackground(int color)
{
    return std::format("\u001B[48;2;{}m", hexColorToAnsi(color));
}

std::string ColorUtility::ansiReset()
{
    return "\033[0m";
}
