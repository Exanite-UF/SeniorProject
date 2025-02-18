#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>

class TextureManager
{
public:
    // TODO: Figure out which of these are actually needed before implementing
    GLuint loadColorTexture(std::string_view path);
    GLuint loadNormalTexture(std::string_view path);
    // TODO: Other formats? Emission is probably color, specular/metallicity/etc are probably packed as a color texture
};
