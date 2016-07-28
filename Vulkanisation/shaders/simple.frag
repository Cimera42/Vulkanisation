#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNorm;
layout (location = 3) in flat int inMaterialIndex;

layout (binding = 1) uniform sampler2DArray textureSamplerArray;
layout (binding = 2) uniform Material
{
    vec3 diffuseColour;
    vec3 specularColour;
    float shininess;
} materials;

layout (location = 0) out vec4 uFragColour;

void main()
{
    float intensity = dot(normalize(vec3(-1,1,-1)), inNorm);
    uFragColour = intensity * texture(textureSamplerArray, vec3(inUV, inMaterialIndex));
}
