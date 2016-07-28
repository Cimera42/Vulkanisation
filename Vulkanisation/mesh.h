#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include "assorted.h" //MemoryBuffer
#include "texture.h" //Texture

struct Vertex
{
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec3 normal;
    int materialIndex;
};

struct MaterialBuffer
{
    glm::vec3 diffuseColour;
    glm::vec3 specularColour;

    float shininess;
};

struct Material
{
    MaterialBuffer materialBuffer;
    std::string texturePath;
};

class Mesh
{
    public:
        MemoryBuffer vertexBuffer;
        MemoryBuffer indexBuffer;
        Texture tex;
        bool textured = false;

        std::vector<Vertex> collated;
        std::vector<uint32_t> indices;
        std::vector<Material> materials;

        std::vector<glm::vec3> vertices;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec3> normals;

        bool vulkan();
        bool loadModel(std::string filepath);
        bool loadWithVectors(std::vector<glm::vec3> inVertices,
                         std::vector<glm::vec2> inUvs,
                         std::vector<glm::vec3> inNormals,
                         std::vector<unsigned int> inIndices);

        void deleteModel();
};

#endif // MESH_H_INCLUDED
