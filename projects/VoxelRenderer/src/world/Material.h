#pragma once

#include <memory>
#include <string>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <src/utilities/NonCopyable.h>
#include <src/utilities/OpenGl.h>

#include <src/graphics/Texture.h>

// Materials are a property of voxels, however, they are also used heavily by the VoxelChunk and VoxelRenderer
// This means we need to account for how they are used before deciding how they should be stored

// In shaders, materials will likely be used in the following manner:
// uniform uint materialMap[4096];
// uniform Material materials[512];

// Every voxel in the world has a material ID. This material ID is 12 bits in total. 2^12 is 4096 material IDs.
// However, this is just the material ID the voxel has.
// To get the actual material of the voxel, the material ID is used to index into the materialMap and get the index of the actual material.
// Therefore, actual material is the following expression: materials[materialMap[materialId]].

// This has the effect of allowing each material to have multiple material IDs.
// This also means that we can have 512 materials in total, assuming we use the exact declaration above.
// We can easily change the size of the material array, given that we don't hit buffer size restrictions.

// This material ID map and material array will need to be stored by the VoxelRenderer (or alternatively, stored once per program instance).
// Why not store this data per VoxelChunk? If we do this, we need to know which material map+array to use.
// Since we only have a G-Buffer with material IDs, we'll need to store additional data to know which chunk the material ID comes from.
// Since we also need to store multiple material maps+arrays, this is very costly for little to no benefit.

// Note that the Material struct that is sent to the GPU may be different from the one stored on the CPU. Call the GPU version: MaterialData.
// MaterialData is fully defined by a Material. Given a Material, a MaterialData can be created, then likely stored into the GPU side material data buffer.

// Similarly to how the material representation on the CPU can be different to the one stored on the GPU, the material representation on disk can be different as well.
// Our material buffers store material IDs (which then map to material indexes), but on disk, we can alternatively store the material indexes directly.
// This way, the exact material stored is not dependent on the material map and the material map can change between program executions.
// However, this increases complexity, and direct storage of mipmapped material IDs to disk should be implemented first.

// TODO: Probably move this to the rendering folder
struct MaterialDefinition
{
    glm::vec3 emission;
    float textureScaleX = 1; // How many voxels large is the texture
    glm::vec3 albedo;
    float textureScaleY = 1; // How many voxels large is the texture
    glm::vec3 metallicAlbedo;
    float padding1;
    float roughness;
    float metallic;

    std::uint64_t albedoTextureID = 0;
    std::uint64_t roughnessTextureID = 0;
    std::uint64_t emissionTextureID = 0;

    // glm::vec3 color;
    //
    // glm::vec2 uvOffset;
    // glm::vec2 uvSize = glm::vec2(1, 1);
    //
    // uint16_t textureIndex;
};

class Material : public NonCopyable
{
    // A material can be uniquely identified either by its index or its key
    // When serializing voxels, voxels should store the integer index to save space
    // When serializing materials, the string key should be saved
    // At least one string key to integer index mapping should be saved, either in the material file or in a separate file

private:
    int16_t index = -1; // Index of the material in the material array
    std::string key; // String key

public:
    std::string name; // Display name

    glm::vec3 emission = glm::vec3(0);
    glm::vec3 albedo = glm::vec3(1);
    glm::vec3 metallicAlbedo = glm::vec3(1);
    glm::vec2 textureScale = glm::vec2(1);
    float roughness = 1;
    float metallic = 0;

    std::shared_ptr<Texture> albedoTexture = nullptr;
    std::shared_ptr<Texture> roughnessTexture = nullptr;
    std::shared_ptr<Texture> emissionTexture = nullptr;

    Material();
    Material(uint16_t index, const std::string& key, const std::string& name = "UNNAMED");

    // glm::vec3 color = glm::vec3(1, 1, 1);

    // glm::vec2 uvOffset = glm::vec2(0, 0); // Offset in UV coordinates. 0.5 is half the texture.
    // glm::vec2 uvSize = glm::vec2(1, 1); // (1, 1) is the size of 1 voxel.

    // GLuint textureId = 0;

    [[nodiscard]] uint16_t getIndex() const;
    [[nodiscard]] const std::string& getKey() const;
};
