// Shadow Vertex Shader, MASKED

#version 460

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec2 out_uv;

layout(std140, set = 1, binding = 0) uniform Camera
{
    mat4 VP;
} camera;

layout(std140, set = 1, binding = 1) uniform Transform
{
    mat4 M;
} transform;

void main()
{
    out_uv = in_uv;

    gl_Position = camera.VP * transform.M * vec4(in_pos, 1.0f);
}
