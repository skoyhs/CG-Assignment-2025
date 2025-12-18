// G-Buffer Vertex Shader

#version 460

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_tangent;
layout(location = 3) out vec3 out_bitangent;

layout(std140, set = 1, binding = 0) uniform Transform
{
    mat4 VP;
} transform;

layout(std140, set = 1, binding = 1) uniform Model
{
    mat4 M;
} model;

void main()
{
    out_uv = in_uv;

    out_normal = (model.M * vec4(in_normal, 0.0f)).xyz;
    out_normal = normalize(out_normal);

    out_tangent = (model.M * vec4(in_tangent, 0.0f)).xyz;
    out_tangent = normalize(out_tangent);

    out_bitangent = cross(out_normal, out_tangent);
    out_tangent = cross(out_bitangent, out_normal);

    gl_Position = transform.VP * model.M * vec4(in_pos, 1.0f);
}
