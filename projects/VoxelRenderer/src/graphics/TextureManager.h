#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <src/graphics/Texture.h>
#include <src/graphics/TextureType.h>
#include <src/utilities/OpenGl.h>
#include <src/utilities/Singleton.h>
#include <src/utilities/TupleHasher.h>

#include <src/graphics/TextureData.h>
// Images that use floats should be supported for HDR textures.
// At the moment the cubemaps load with floats

class TextureManager : public Singleton<TextureManager>
{
private:
    std::mutex dataMtx; // Prevents difference threads from accessing texturesWithoutBindlessHandles and textures simultaneously

    // (path, format) -> texture
    bool _areBindlessTexturesEnabled = false;
    std::unordered_set<std::shared_ptr<Texture>> texturesWithoutBindlessHandles;
    std::unordered_map<std::tuple<std::string, GLenum>, std::shared_ptr<Texture>, TupleHasher<std::tuple<std::string, GLenum>>> textures;

    static GLenum getOpenGlStorageFormat(TextureType type);
    static int getFormatChannelCount(GLenum storageFormat);

    std::shared_ptr<Texture> loadTexture(std::string_view path, TextureType type, GLenum storageFormat);

    // Path points to a file with a list of file names for the 6 faces
    /*This is the order the files need to be listed
    right.jpg
    left.jpg
    top.jpg
    bottom.jpg
    front.jpg
    back.jpg
    */
    std::shared_ptr<Texture> loadCubemapTexture(std::string_view path, TextureType type, GLenum storageFormat);

public:
    // Loads a texture using a texture type preset
    std::shared_ptr<Texture> loadTexture(std::string_view path, TextureType type);

    // Loads a texture with the specified format and colorspace
    std::shared_ptr<Texture> loadTexture(std::string_view path, GLenum storageFormat);

    // Loads a texture using a texture type preset
    std::shared_ptr<Texture> loadCubemapTexture(std::string_view path, TextureType type);

    // Loads a texture with the specified format and colorspace
    std::shared_ptr<Texture> loadCubemapTexture(std::string_view path, GLenum storageFormat);

    // Makes bindless handles for textures that do not already have handles
    void makeBindlessTextureHandles();

    // Remakes all bindless texture handles, must be done when switching on and off asynchronous reprojection
    void scheduleRemakeBindlessTextureHandles();

    bool areBindlessTexturesEnabled() const;

    // This exists so that bindless textures can be disabled by default
    // If this is not run, then RenderDoc will work
    void enableBindlessTextures();

    template<class T>
    std::shared_ptr<Texture> loadTexture(std::string_view name, std::shared_ptr<TextureData<T>> data){
        std::scoped_lock lock(dataMtx);
        // Use cached texture if available
        auto cacheKey = std::make_tuple(std::string(name), data->getInternalFormat());
        if (textures.contains(cacheKey))
        {
            //If the Texture data is not dirty, then use the cached texture
            if(!data->isDirty){
                return textures[cacheKey];
            }
            //Else, remake the texture

            //First delete the existing texture
            //Get the texture id of the existing texture
            GLuint textureID = textures[cacheKey]->getTextureId();
            glDeleteTextures(1, &textureID);//Delete that texture
        }


        // Create OpenGL texture
        GLuint textureId;
        glGenTextures(1, &textureId);

        glBindTexture(GL_TEXTURE_2D, textureId);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);//The cpu data is tightly packed, if this is normally expecting a byte alignment of 4

            // internalFormat (called storageFormat here) is the format of the texture stored on the GPU
            // format is the input format, as loaded from the texture file
            glTexImage2D(GL_TEXTURE_2D, 0, data->getInternalFormat(), data->getWidth(), data->getHeight(), 0, data->getDataFormat(), data->getDataType(), data->data());
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, data->getWidth(), data->getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, data->data());
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // Wrap OpenGL handle with the Texture class
        
        data->isDirty = false;
        // Insert texture into cache
        if (textures.contains(cacheKey))
        {
            //Since we already have a Texture in the cache, we need to modify the existing Texture
            //Otherwise we will break the materials that use this Texture
            auto& texture = textures[cacheKey];
            texture->textureId = textureId;//Replace the texture id
            texture->bindlessHandle = 0;
            texture->size = glm::ivec2(data->getWidth(), data->getHeight());
            texture->type = Unknown;
            texture->_isCubemap = false;
            texturesWithoutBindlessHandles.insert(texture);
            return texture;
        }else{
            auto texture = std::make_shared<Texture>(textureId, Unknown, glm::ivec2(data->getWidth(), data->getHeight()), false);
            textures[cacheKey] = texture;
            texturesWithoutBindlessHandles.insert(texture);
            return texture;
        }
    }
};
