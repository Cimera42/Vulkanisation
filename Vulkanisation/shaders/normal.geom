#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout (location = 0) in vec3 inNorm[];

layout (location = 0) out vec3 outNorm;

layout (binding = 0) uniform UniformBuffer
{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 modelMatrix;
} uniformBuffer;

void main()
{
	float normalLength = 0.2;
	for(int i = 0; i < gl_in.length(); i++)
	{
		vec3 pos = gl_in[i].gl_Position.xyz;

		vec3 start = pos;
		gl_Position = uniformBuffer.projectionMatrix *
                      uniformBuffer.viewMatrix *
                      uniformBuffer.modelMatrix *
                      vec4(start, 1.0);

        outNorm = normalize(inNorm[i] * (inverse(mat3(uniformBuffer.modelMatrix))));;
		EmitVertex();

		vec3 end = pos + inNorm[i]*normalLength;
		gl_Position = uniformBuffer.projectionMatrix *
                      uniformBuffer.viewMatrix *
                      uniformBuffer.modelMatrix *
                      vec4(end, 1.0);

        outNorm = normalize(inNorm[i] * (inverse(mat3(uniformBuffer.modelMatrix))));;
		EmitVertex();

        EndPrimitive();
	}
}
