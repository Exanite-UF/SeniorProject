#pragma once

#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

#include <src/utilities/NonCopyable.h>
#include <src/utilities/OpenGl.h>

// A templated buffer class.
//
// Note that a buffer can be bound to any buffer target at any time.
// Eg: A buffer created using GL_SHADER_STORAGE_BUFFER can also be used as an ARRAY_BUFFER or any other buffer target.
template <typename T>
class GraphicsBuffer : public NonCopyable
{
private:
    int bindLocation = 0;

    uint64_t elementCount = 0;

public:
    GLuint bufferId = 0;

    GraphicsBuffer();
    explicit GraphicsBuffer(uint64_t elementCount);
    ~GraphicsBuffer();

    uint64_t getByteCount() const;
    uint64_t getElementCount() const;

    void setSize(std::uint64_t elementCount);

    void readFrom(std::span<const T> data, uint32_t elementOffset = 0);
    void writeTo(std::span<T> data, uint32_t elementOffset);

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
uint64_t GraphicsBuffer<T>::getByteCount() const
{
    return elementCount * sizeof(T);
}

template <typename T>
uint64_t GraphicsBuffer<T>::getElementCount() const
{
    return elementCount;
}

template <typename T>
void GraphicsBuffer<T>::setSize(std::uint64_t elementCount)
{
    if (this->elementCount == elementCount)
    {
        return;
    }

    this->elementCount = elementCount;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId); // Bind the buffer so we can set the data
    glBufferData(GL_SHADER_STORAGE_BUFFER, elementCount * sizeof(T), nullptr, GL_DYNAMIC_COPY); // Set the data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind the buffer
}
template <typename T>
void GraphicsBuffer<T>::readFrom(std::span<const T> data, uint32_t elementOffset)
{
    if (data.size() + elementOffset > elementCount)
    {
        throw std::runtime_error("data.size() + elementOffset is greater than GraphicsBuffer.elementCount");
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId); // Bind the buffer so we can set the data
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, elementOffset * sizeof(T), data.size() * sizeof(T), static_cast<void*>(const_cast<T*>(data.data()))); // Set the data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind the buffer
}

template <typename T>
void GraphicsBuffer<T>::writeTo(std::span<T> data, uint32_t elementOffset)
{
    if (data.size() + elementOffset > elementCount)
    {
        throw std::runtime_error("data.size() + elementOffset is greater than GraphicsBuffer.elementCount");
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId); // Bind the buffer so we can get the data
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, elementOffset * sizeof(T), data.size() * sizeof(T), static_cast<void*>(data.data())); // Get the data
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
