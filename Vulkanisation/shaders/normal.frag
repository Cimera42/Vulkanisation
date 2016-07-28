#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inNorm;

layout (location = 0) out vec4 uFragColour;

void main()
{
    uFragColour = vec4((inNorm+1)/2,1);
}
