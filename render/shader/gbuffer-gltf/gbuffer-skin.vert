// G-Buffer Vertex Shader, rigged

#version 460

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;
layout(location = 4) in uvec4 in_joint_indices;
layout(location = 5) in vec4 in_joint_weights;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_tangent;
layout(location = 3) out vec3 out_bitangent;

layout(std140, set = 0, binding = 0) readonly buffer Joints
{
    mat4 joint_matrices[];
};

layout(std140, set = 1, binding = 0) uniform Transform
{
    mat4 VP;
} transform;

layout(std140, set = 1, binding = 1) uniform Joint_param
{
    uint offset;
} joint_params;

void main()
{
    out_uv = in_uv;

    mat4 joint_matrix_0 = joint_matrices[in_joint_indices.x + joint_params.offset];
    mat4 joint_matrix_1 = joint_matrices[in_joint_indices.y + joint_params.offset];
    mat4 joint_matrix_2 = joint_matrices[in_joint_indices.z + joint_params.offset];
    mat4 joint_matrix_3 = joint_matrices[in_joint_indices.w + joint_params.offset];

    mat4 skin_matrix =
        joint_matrix_0 * in_joint_weights.x +
            joint_matrix_1 * in_joint_weights.y +
            joint_matrix_2 * in_joint_weights.z +
            joint_matrix_3 * in_joint_weights.w;

    out_normal = (skin_matrix * vec4(in_normal, 0.0f)).xyz;
    out_normal = normalize(out_normal);

    out_tangent = (skin_matrix * vec4(in_tangent, 0.0f)).xyz;
    out_tangent = normalize(out_tangent);

    out_bitangent = cross(out_normal, out_tangent);
    out_tangent = cross(out_bitangent, out_normal);

    gl_Position = transform.VP * skin_matrix * vec4(in_pos, 1.0f);
}
