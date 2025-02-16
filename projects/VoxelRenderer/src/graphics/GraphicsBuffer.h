#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>

#include <cstdint>

// A templated buffer class.
//
// Note that a buffer can be bound to any buffer target at any time.
// Eg: A buffer created using GL_SHADER_STORAGE_BUFFER can also be used as an ARRAY_BUFFER or any other buffer target.
template <typename T>
class GraphicsBuffer
{
private:
    // TODO: Consider removing this and the associated bind/unbind methods
    int bindLocation;

    uint64_t elementCount = 0;

public:
    GLuint bufferId;

    GraphicsBuffer();
    explicit GraphicsBuffer(uint64_t elementCount);
    ~GraphicsBuffer();

    uint64_t getByteCount();
    uint64_t getElementCount();

    void setSize(std::uint64_t elementCount);

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
    glGenBuffers(1, &bufferId);
}

template <typename T>
GraphicsBuffer<T>::GraphicsBuffer(uint64_t elementCount)
{
    glGenBuffers(1, &bufferId);
    setSize(elementCount);
}

template <typename T>
GraphicsBuffer<T>::~GraphicsBuffer()
{
    glDeleteBuffers(1, &bufferId);
}

template <typename T>
uint64_t GraphicsBuffer<T>::getByteCount()
{
    return elementCount * sizeof(T);
}

template <typename T>
uint64_t GraphicsBuffer<T>::getElementCount()
{
    return elementCount;
}

template <typename T>
void GraphicsBuffer<T>::setSize(std::uint64_t elementCount)
{
    this->elementCount = elementCount;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId); // Bind the buffer so we can set the data
    glBufferData(GL_SHADER_STORAGE_BUFFER, elementCount * sizeof(T), nullptr, GL_DYNAMIC_COPY); // Set the data
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
