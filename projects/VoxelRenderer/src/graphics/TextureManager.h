#pragma once

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

#include <src/graphics/Texture.h>
#include <src/graphics/TextureType.h>
#include <src/utilities/OpenGl.h>
#include <src/utilities/Singleton.h>
#include <src/utilities/TupleHasher.h>

// Images that use floats should be supported for HDR textures.
// At the moment the cubemaps load with floats

class TextureManager : public Singleton<TextureManager>
{
private:
    std::mutex dataMtx;//Prevents difference threads from accessing texturesWithoutBindlessHandles and textures simultaneously

    // (path, format) -> texture
    bool _areBindlessTexturesEnabled = false;
    std::unordered_set<std::shared_ptr<Texture>> texturesWithoutBindlessHandles;
    std::unordered_map<std::tuple<std::string_view, GLenum>, std::shared_ptr<Texture>, TupleHasher<std::tuple<std::string_view, GLenum>>> textures;

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

    //Makes bindless handles for textures that do not already have handles
    void makeBindlessTextureHandles();

    //Remakes all bindless texture handles, must be done when switching on and off asynchronous reprojection
    void scheduleRemakeBindlessTextureHandles();

    bool areBindlessTexturesEnabled() const;

    //This exists so that bindless textures can be disabled by default
    //If this is not run, then RenderDoc will work
    void enableBindlessTextures();
};
