#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNorm;
layout (location = 3) in int inMaterialIndex;

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNorm;
layout (location = 3) out int outMaterialIndex;

layout (binding = 0) uniform UniformBuffer
{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 modelMatrix;
} uniformBuffer;

void main()
{
    outPos = (uniformBuffer.modelMatrix * vec4(inPos, 1.0)).xyz;
    outUV = vec2(inUV.x, 1-inUV.y);
    outNorm = normalize(inNorm * (inverse(mat3(uniformBuffer.modelMatrix))));
    outMaterialIndex = inMaterialIndex;

    gl_Position = uniformBuffer.projectionMatrix *
                  uniformBuffer.viewMatrix *
                  uniformBuffer.modelMatrix *
                  vec4(inPos, 1.0);
}
