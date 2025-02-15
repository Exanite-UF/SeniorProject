#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>

#include <cstdint>

template <typename T>
class GraphicsBuffer
{
private:
    // TODO: Consider removing this and the associated bind/unbind methods
    int bindLocation;

public:
    GLuint bufferId;

    GraphicsBuffer();
    ~GraphicsBuffer();

    void resize(std::uint64_t size);

    // Binds to a specific location
    void bind(int location);

    // Unbinds the buffer
    // It remembers the location it was bound to
    // Warning: This assumes that this buffer is bound. If not, it will unbind whatever is bound
    void unbind() const;
};

template <typename T>
GraphicsBuffer<T>::GraphicsBuffer()
{
    glGenBuffers(1, &bufferId); // Create the buffer
}

template <typename T>
GraphicsBuffer<T>::~GraphicsBuffer()
{
    glDeleteBuffers(1, &bufferId);
}

template <typename T>
void GraphicsBuffer<T>::resize(std::uint64_t size)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId); // Bind the buffer so we can set the data
    glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(T), nullptr, GL_DYNAMIC_COPY); // Set the data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind the buffer
}

template <typename T>
void GraphicsBuffer<T>::bind(int location)
{
    bindLocation = location;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindLocation, bufferId);
}

template <typename T>
void GraphicsBuffer<T>::unbind() const
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindLocation, 0);
}
