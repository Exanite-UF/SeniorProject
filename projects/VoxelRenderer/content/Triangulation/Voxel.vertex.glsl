#version 330 core
layout(location = 0) in vec3 aPos; // Cube vertex positions
layout(location = 1) in vec3 instancePos; // Instance position for each voxel
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;
  
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

void main()
{
    vec4 worldPosition = model * (vec4(aPos, 1.0) + vec4(instancePos, 0.0)); // Add per-instance position
    gl_Position = projection * view * worldPosition;
    FragPos = vec3(worldPosition); 
    //Normal = mat3(transpose(inverse(model))) * aNormal; //ineficient but good for now
    Normal = mat3(model) * aNormal;
    //gl_Position.z = 2.0 * log(gl_Position.w/0.001)/log(1000/0.001) - 1;
    //gl_Position.z *= gl_Position.w;
    TexCoord = aTexCoord;
}