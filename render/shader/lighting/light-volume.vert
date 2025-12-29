#version 460

layout(location = 0) in vec3 position;

layout(std140, set = 1, binding = 0) uniform Param
{
    mat4 MVP;
};

void main()
{
    gl_Position = MVP * vec4(position, 1.0);
}
