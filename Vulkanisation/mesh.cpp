#include "mesh.h"
#include <iostream>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flag

#include "vulkanDefinitions.h"
#include "assorted.h"

void Mesh::deleteModel()
{
    vertexBuffer.destroy();
    indexBuffer.destroy();
    if(textured)
        tex.destroy();
}

bool Mesh::vulkan()
{
    if(!createBuffer(sizeof(Vertex) * collated.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, collated.data()
                    ,&vertexBuffer))
    {
        return false;
    }

    if(!createBuffer(sizeof(uint32_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices.data()
                    ,&indexBuffer))
    {
        return false;
    }

    std::vector<std::string> texPaths;
    for(int i = 0; i < materials.size(); i++)
    {
        if(materials[i].texturePath.find_first_not_of(' ') != std::string::npos)
        {
            std::string backslashFixed = materials[i].texturePath;
            std::replace(backslashFixed.begin(), backslashFixed.end(), '\\', '/');
            texPaths.push_back(backslashFixed);
        }
    }
    if(texPaths.size() > 0)
    {
        textured = true;
        tex.loadTextureArray(texPaths);
    }

    return true;
}

bool Mesh::loadModel(std::string filepath)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath.c_str(),
                                             aiProcess_CalcTangentSpace |
                                             aiProcess_Triangulate |
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_SortByPType |
                                             aiProcess_PreTransformVertices);

    if(!scene)
    {
        std::cout << "Could not load model. Error importing" << std::endl;
        return false;
    }

    for(int i = 0; i < scene->mNumMaterials; i++)
    {
        aiMaterial* assimpMaterial = scene->mMaterials[i];

        aiString nnn;
        assimpMaterial->Get(AI_MATKEY_NAME, nnn);
        if(strcmp(nnn.C_Str(), AI_DEFAULT_MATERIAL_NAME) == 0)
            continue;
        std::cout << "matname: " << nnn.C_Str() << std::endl;

        Material material;
        aiColor3D dcol;
        assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, dcol);
        material.materialBuffer.diffuseColour = glm::vec3(dcol.r,dcol.g,dcol.b);

        aiColor3D scol;
        assimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, scol);
        material.materialBuffer.diffuseColour = glm::vec3(scol.r,scol.g,scol.b);

        assimpMaterial->Get(AI_MATKEY_SHININESS, material.materialBuffer.shininess);

        aiString texPath;
        if(assimpMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
        {
            material.texturePath = std::string(texPath.C_Str());
            std::cout << "texpath: " <<  material.texturePath << std::endl;
        }

        materials.push_back(material);
    }

    int indexOffset = 0;
    for(int i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* assimpMesh = scene->mMeshes[i];

        for(int j = 0; j < assimpMesh->mNumFaces; j++)
        {
            aiFace& assimpFace = assimpMesh->mFaces[j];

            for(int k = 0; k < assimpFace.mNumIndices; k++)
            {
                indices.push_back(assimpFace.mIndices[k] + indexOffset);
            }
        }
        for(int j = 0; j < assimpMesh->mNumVertices; j++)
        {
            aiVector3D vertex = assimpMesh->mVertices[j];
            glm::vec3 glmVert = glm::vec3(vertex.x,vertex.y,vertex.z);
            vertices.push_back(glmVert);

            aiVector3D uv = assimpMesh->mTextureCoords[0][j];
            glm::vec2 glmUv = glm::vec2(uv.x,uv.y);
            uvs.push_back(glmUv);

            aiVector3D normal = assimpMesh->mNormals[j];
            glm::vec3 glmNormal = glm::vec3(normal.x,normal.y,normal.z);
            normals.push_back(glmNormal);

            Vertex collatedVertex;
            collatedVertex.pos = glmVert;
            collatedVertex.uv = glmUv;
            collatedVertex.normal = glmNormal;
            //obj material index starts at 1
            if(filepath.find("obj") != std::string::npos)
                collatedVertex.materialIndex = assimpMesh->mMaterialIndex-1;
            else
                collatedVertex.materialIndex = assimpMesh->mMaterialIndex;
            collated.push_back(collatedVertex);
        }
        indexOffset += assimpMesh->mNumVertices;
    }

    return vulkan();
}

bool Mesh::loadWithVectors(std::vector<glm::vec3> inVertices, std::vector<glm::vec2> inUvs, std::vector<glm::vec3> inNormals, std::vector<unsigned int> inIndices)
{
    vertices.swap(inVertices);
    uvs.swap(inUvs);
    normals.swap(inNormals);
    indices.swap(inIndices);

    for(int i = 0; i < vertices.size(); i++)
    {
        Vertex collatedVertex;
        collatedVertex.pos = vertices[i];
        collatedVertex.uv = uvs[i];
        collatedVertex.normal = normals[i];
        collated.push_back(collatedVertex);
    }

    return vulkan();
}
