#pragma once

#include <src/utilities/OpenGl.h>
#include <src/utilities/NonCopyable.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <stdexcept>
#include <type_traits>
#include <memory>
#include <iostream>

#include <glm/glm.hpp>


    
extern std::unordered_map<GLenum, int> internalFormatToChannels;

template<typename T>
class TextureData : public NonCopyable, public std::enable_shared_from_this<TextureData<T>> {
private:
    static_assert(std::is_arithmetic<T>::value, "TextureData<T>: T must be a numeric type");
    static_assert(!std::is_same<T, double>::value, "TextureData<T>: T must not be a double");


    bool isDirty = true;
    std::vector<T> pixelData;

    int width = -1;
    int height = -1;
    int channels = -1;
    GLenum internalFormat;

    TextureData(int width, int height, GLenum internalFormat);

    friend class TextureManager;
public:

    static std::shared_ptr<TextureData<T>> makeTextureData(int width, int height, GLenum internalFormat);

    GLenum getInternalFormat() const;
    GLenum getDataFormat() const;
    GLenum getDataType() const;

    const T* data() const;

    int getWidth() const;
    int getHeight() const;

    //Sets the pixel data, ignoring any extra data provided
    void setPixel(int x, int y, T r, T g = 0, T b = 0, T a = 0);
    std::vector<T> getPixel(int x, int y);
    
    template<typename U = T, typename = std::enable_if_t<std::is_floating_point<U>::value>>
    void setPixel(int x, int y, glm::vec1 color){
        setPixel(x, y, color.r);
    }

    template<typename U = T, typename = std::enable_if_t<std::is_floating_point<U>::value>>
    void setPixel(int x, int y, glm::vec2 color){
        setPixel(x, y, color.r, color.g);
    }

    template<typename U = T, typename = std::enable_if_t<std::is_floating_point<U>::value>>
    void setPixel(int x, int y, glm::vec3 color){
        setPixel(x, y, color.r, color.g, color.b);
    }

    template<typename U = T, typename = std::enable_if_t<std::is_floating_point<U>::value>>
    void setPixel(int x, int y, glm::vec4 color){
        setPixel(x, y, color.r, color.g, color.b, color.a);
    }


    template<typename U = T, typename = std::enable_if_t<std::is_integral<U>::value>>
    void setPixel(int x, int y, glm::ivec1 color){
        setPixel(x, y, color.r);
    }

    template<typename U = T, typename = std::enable_if_t<std::is_integral<U>::value>>
    void setPixel(int x, int y, glm::ivec2 color){
        setPixel(x, y, color.r, color.g);
    }

    template<typename U = T, typename = std::enable_if_t<std::is_integral<U>::value>>
    void setPixel(int x, int y, glm::ivec3 color){
        setPixel(x, y, color.r, color.g, color.b);
    }

    template<typename U = T, typename = std::enable_if_t<std::is_integral<U>::value>>
    void setPixel(int x, int y, glm::ivec4 color){
        setPixel(x, y, color.r, color.g, color.b, color.a);
    }
    
};

template <typename T>
inline TextureData<T>::TextureData(int width, int height, GLenum internalFormat)
{
    if(width <= 0){
        throw std::runtime_error("Invalid width. Must be 1 or greater.");
    }

    if(height <= 0){
        throw std::runtime_error("Invalid height. Must be 1 or greater.");
    }

    if(internalFormatToChannels.count(internalFormat) == 0){
        throw std::runtime_error("Invalid internal format. Check specification of glTexImage2D.");
    }

    this->width = width;
    this->height = height;
    this->internalFormat = internalFormat;

    this->channels = internalFormatToChannels.at(internalFormat);
    pixelData.resize(width * height * channels, 0);
}

template <typename T>
inline std::shared_ptr<TextureData<T>> TextureData<T>::makeTextureData(int width, int height, GLenum internalFormat)
{
    return std::shared_ptr<TextureData<T>>(new TextureData<T>(width, height, internalFormat));
}

template <typename T>
inline GLenum TextureData<T>::getInternalFormat() const
{
    return internalFormat;
}


template <typename T>
inline GLenum TextureData<T>::getDataFormat() const
{
    static constexpr GLenum types[4] = {GL_RED, GL_RG, GL_RGB, GL_RGBA};
    return types[channels - 1];
}

template <typename T>
inline GLenum TextureData<T>::getDataType() const {
    if constexpr (std::is_integral<T>::value) {
        int size = sizeof(T);

        if(size <= 0 || size > 4){
            throw std::runtime_error("GPU's do not support integral types of that size.");
        }
        if constexpr (std::is_signed<T>::value) {
            static constexpr GLenum types[4] = {GL_BYTE, GL_SHORT, GL_INVALID_ENUM, GL_INT};
            return types[size - 1];  // Signed integers 
        } else {
            static constexpr GLenum types[4] = {GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_INVALID_ENUM, GL_UNSIGNED_INT};
            return types[size - 1];  // Unsigned integers
        }
    } else {
        return GL_FLOAT;  // Default to GL_FLOAT for non-integral types (e.g., float)
    }
}

template<typename T>
inline const T* TextureData<T>::data() const
{
    return pixelData.data();
}

template<typename T>
inline int TextureData<T>::getWidth() const
{
    return width;
}

template <typename T>
inline int TextureData<T>::getHeight() const
{
    return height;
}

template <typename T>
inline void TextureData<T>::setPixel(int x, int y, T r, T g, T b, T a)
{
    if(x < 0 || y < 0 || x >= width || y >= height){
        throw std::out_of_range("x, y must be within the bounds of the texture.");
    }

    int index = channels * (x + width * y);
    T data[4] = {r, g, b, a};

    for(int i = 0; i < channels; i++){
        pixelData[index + i] = data[i];
    }

    isDirty = true;
}

template <typename T>
inline std::vector<T> TextureData<T>::getPixel(int x, int y)
{
    if(x < 0 || y < 0 || x >= width || y >= height){
        throw std::out_of_range("x, y must be within the bounds of the texture.");
    }

    std::vector<T> output;
    output.reserve(channels);

    int index = channels * (x + width * y);
    for(int i = 0; i < channels; i++){
        output.push_back(pixelData[index + i]);
    }
    return output;
}
