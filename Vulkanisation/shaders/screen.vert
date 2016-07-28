#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outPos;
layout (location = 1) out vec2 outUV;

layout (binding = 0) uniform UniformBuffer
{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 modelMatrix;
} uniformBuffer;

void main()
{
    outUV = vec2(inUV.x, 1-inUV.y);

    outPos = uniformBuffer.projectionMatrix *
                  vec4(inPos, 1.0);
    outPos.y = -outPos.y;
    gl_Position = outPos;
}
