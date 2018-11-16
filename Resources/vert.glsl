#version 450 core

layout (set = 0, binding = 0) uniform ModelMatrix {
    mat4 model;
} modelMatrix;

layout (push_constant) uniform ViewProjection {
    mat4 view;
    mat4 projection;
} viewProjection;

layout (location = 0) in vec3 inPos;

layout (location = 1) in vec3 inNor;

layout (location = 2) in vec2 inUV;

layout (location = 3) in vec3 inTan;

layout (location = 4) in vec3 inBitan;

layout (location = 0) out vec3 fragPos;

layout (location = 1) out vec2 fragUV;

layout (location = 2) out vec3 fragNor;

layout (location = 3) out vec3 fragTan;

layout (location = 4) out vec3 fragBitan;


void main()
{
    fragPos     = vec3(viewProjection.view * modelMatrix.model * vec4(inPos, 1.0));
    fragNor     = mat3(viewProjection.view * modelMatrix.model) * inNor;
    fragUV      = inUV;
    fragTan     = mat3(viewProjection.view * modelMatrix.model) * inTan;
    fragBitan   = mat3(viewProjection.view * modelMatrix.model) * inBitan;


    gl_Position = viewProjection.projection * vec4(fragPos, 1.0);
}