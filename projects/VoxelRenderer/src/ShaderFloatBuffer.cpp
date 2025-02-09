#include "ShaderFloatBuffer.h"

ShaderFloatBuffer::ShaderFloatBuffer()
{
    glGenBuffers(1, &bufferID); // Create the buffer
}

ShaderFloatBuffer::~ShaderFloatBuffer()
{
    glDeleteBuffers(1, &bufferID);
}

void ShaderFloatBuffer::setSize(std::uint64_t size)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferID); // Bind the buffer so we can set the data
    glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(float), nullptr, GL_DYNAMIC_COPY); // Set the data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind the buffer
}

void ShaderFloatBuffer::bind(int index)
{
    bindLocation = index;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindLocation, bufferID);
}

void ShaderFloatBuffer::unbind() const
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindLocation, 0);
}
