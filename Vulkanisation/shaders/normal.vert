#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNorm;

layout (location = 0) out vec3 outNorm;

void main()
{
    outNorm = inNorm;
    gl_Position = vec4(inPos, 1.0);
}
