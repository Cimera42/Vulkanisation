#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec2 inUV;

layout (binding = 1) uniform sampler2D textureSampler;

layout (location = 0) out vec4 uFragColour;

void main()
{
    uFragColour = texture(textureSampler, vec2(inUV.x , inUV.y));
}
