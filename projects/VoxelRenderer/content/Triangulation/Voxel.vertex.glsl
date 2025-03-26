#version 330 core
layout(location = 0) in vec3 aPos; // Cube vertex positions
layout(location = 1) in vec3 instancePos; // Instance position for each voxel

uniform mat4 projection;
uniform mat4 view;

void main()
{
    mat4 model = glm::translate(mat4(1.0f), instancePos); // Translate each voxel to its position
    gl_Position = projection * view * model * vec4(aPos, 1.0f); // Apply transformations
}