#include "ShaderByteBuffer.h"

ShaderByteBuffer::ShaderByteBuffer(){
    glGenBuffers(1, &bufferID);//Create the buffer
}

ShaderByteBuffer::~ShaderByteBuffer(){
    glDeleteBuffers(1, &bufferID);
}

void ShaderByteBuffer::setSize(std::uint64_t size)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferID);//Bind the buffer so we can set the data
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_COPY);//Set the data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);//Unbind the buffer
}


void ShaderByteBuffer::bind(int index){
    bindLocation = index;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, bufferID);
}

void ShaderByteBuffer::unbind() const{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindLocation, 0);
}