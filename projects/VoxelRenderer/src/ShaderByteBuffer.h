#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>

#include <cstdint>

class ShaderByteBuffer
{
private:
    int bindLocation;

public:
    GLuint bufferID;

    ShaderByteBuffer();
    ~ShaderByteBuffer();

    void setSize(std::uint64_t size);

    // Binds to a specific location
    void bind(int index);

    // Unbinds the buffer
    // It remembers the location it was bound to
    // Warning: This assumes that this buffer is bound. If not, it will unbind whatever is bound
    void unbind() const;
};
