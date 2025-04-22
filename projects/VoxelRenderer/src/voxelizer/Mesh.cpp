#include <src/utilities/OpenGl.h>
#include <src/voxelizer/Mesh.h>

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<TriangleTexture> textures)
{
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;

    setupMesh();
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, uv)));

    glBindVertexArray(0);
}

void Mesh::draw(const std::shared_ptr<ShaderProgram>& shader, glm::vec3 cameraPosition, glm::vec3 cameraForwardDirection, glm::vec3 cameraUpDirection, glm::ivec2 windowSize, float zoom)
{
    shader->use();

    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    for (unsigned int i = 0; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        std::string number;
        std::string name = textures[i].type;
        if (name == "texture_diffuse")
        {
            number = std::to_string(diffuseNr++);
        }
        else if (name == "texture_specular")
        {
            number = std::to_string(specularNr++);
        }
        shader->setInt("material." + name + number, i);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    glActiveTexture(GL_TEXTURE0);

    // Camera Setup
    glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + cameraForwardDirection, cameraUpDirection);
    //glm::mat4 projection = glm::perspective(glm::radians(60.0f), static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y), 0.001f, 1000.0f);
    glm::mat4 projection = glm::ortho(
        -zoom * static_cast<float>(windowSize.x) / windowSize.y,  // Left
        zoom * static_cast<float>(windowSize.x) / windowSize.y,   // Right
        -zoom,                                                   // Bottom
        zoom,                                                    // Top
        0.001f, 1000.0f                                               // Near and far clipping planes
    );
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader->programId, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shader->programId, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader->programId, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    //// Material Settings for Phong Shader
    glm::vec3 diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 emissiveColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glUniform3fv(glGetUniformLocation(shader->programId, "defaultMaterial.diffuseColor"), 1, glm::value_ptr(diffuseColor));
    glUniform3fv(glGetUniformLocation(shader->programId, "defaultMaterial.specularColor"), 1, glm::value_ptr(specularColor));
    glUniform3fv(glGetUniformLocation(shader->programId, "defaultMaterial.emissiveColor"), 1, glm::value_ptr(emissiveColor));

    float materialSpecFactor = 0.0f;
    float materialEmisFactor = 0.0f;
    float materialShininess = 32.0f;
    glUniform1f(glGetUniformLocation(shader->programId, "defaultMaterial.specularFactor"), materialSpecFactor);
    glUniform1f(glGetUniformLocation(shader->programId, "defaultMaterial.emissiveFactor"), materialEmisFactor);
    glUniform1f(glGetUniformLocation(shader->programId, "defaultMaterial.shininess"), materialShininess);

    glBindVertexArray(VAO);
    {
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}
