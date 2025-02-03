#include "GraphicsUtils.h"

GLuint GraphicsUtils::emptyVertexArray = 0;


GLuint GraphicsUtils::create3DImage(int width, int height, int depth, GLenum internalFormat, GLenum format, GLenum type)
{
    GLuint img;
    glGenTextures(1, &img);

    glBindTexture(GL_TEXTURE_3D, img);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage3D(
        GL_TEXTURE_3D, 0, internalFormat,
        width, height, depth, // Dimensions for new mip level
        0, format, type, nullptr);

    glBindTexture(GL_TEXTURE_3D, 0);
    return img;
}

GLuint GraphicsUtils::getEmptyVertexArray()
{
    if(emptyVertexArray == 0){
        glGenVertexArrays(1, &emptyVertexArray);
    }
    return emptyVertexArray;
}
