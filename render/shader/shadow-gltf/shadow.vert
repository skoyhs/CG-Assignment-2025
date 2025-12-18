// Shadow Vertex Shader

#version 460

layout(location = 0) in vec3 in_pos;

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
    gl_Position = camera.VP * transform.M * vec4(in_pos, 1.0f);
}
