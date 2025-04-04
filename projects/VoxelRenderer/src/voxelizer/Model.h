#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <src/voxelizer/Mesh.h>

#include <assimp/Importer.hpp> // C++ importer interface
#include <assimp/postprocess.h> // Post processing flags
#include <assimp/scene.h> // Output data structure

unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma = false);

struct TriangleMaterial
{
    glm::vec3 diffuse {};
    glm::vec3 specular {};
    glm::vec3 ambient {};
    float shininess {};
};

class Model
{
public:
    std::vector<TriangleTexture> textures_loaded {};

    explicit Model(const std::string& path);

    void draw(const std::shared_ptr<ShaderProgram>& shader, glm::vec3 cameraPosition, glm::vec3 cameraForwardDirection, glm::vec3 cameraUpDirection, int windowWidth, int windowHeight);
    std::vector<Mesh> meshes; // make private
    std::vector<Triangle> getTriangles();

private:
    std::string directory;
    std::vector<Triangle> triangles;

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<TriangleTexture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
    TriangleMaterial loadMaterial(aiMaterial* mat);
};
