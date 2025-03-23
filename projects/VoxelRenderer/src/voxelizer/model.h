#pragma once

#include <src/voxelizer/mesh.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma = false);

struct Material {
    glm::vec3 Diffuse;
    glm::vec3 Specular;
    glm::vec3 Ambient;
    float Shininess;
};

class Model
{
public:
    std::vector<Texture> textures_loaded;
    Model(char* path)
    {
        loadModel(path);
    }
    void Draw(Shader &shader);
private:
    std::vector<Mesh> meshes;
    std::string directory;

    void loadModel(std::string path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
    Material loadMaterial(aiMaterial* mat);
};