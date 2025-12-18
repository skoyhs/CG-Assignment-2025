// Shadow Vertex Shader, MASKED

#version 460

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in uvec4 in_joint_indices;
layout(location = 3) in vec4 in_joint_weights;

layout(location = 0) out vec2 out_uv;

layout(std430, set = 0, binding = 0) readonly buffer Joints
{
    mat4 joint_matrices[];
};

layout(std140, set = 1, binding = 0) uniform Camera
{
    mat4 VP;
} camera;

layout(std140, set = 1, binding = 1) uniform Joint_param
{
    uint offset;
} joint_params;

void main()
{
    mat4 joint_matrix_0 = joint_matrices[in_joint_indices.x + joint_params.offset];
    mat4 joint_matrix_1 = joint_matrices[in_joint_indices.y + joint_params.offset];
    mat4 joint_matrix_2 = joint_matrices[in_joint_indices.z + joint_params.offset];
    mat4 joint_matrix_3 = joint_matrices[in_joint_indices.w + joint_params.offset];

    mat4 skin_matrix =
        joint_matrix_0 * in_joint_weights.x +
            joint_matrix_1 * in_joint_weights.y +
            joint_matrix_2 * in_joint_weights.z +
            joint_matrix_3 * in_joint_weights.w;

    out_uv = in_uv;
    gl_Position = camera.VP * skin_matrix * vec4(in_pos, 1.0f);
}
