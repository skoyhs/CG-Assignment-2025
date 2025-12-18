// Shadow Fragment Shader, MASKED

#version 460

layout(location = 0) in vec2 out_uv;

layout(set = 2, binding = 0) uniform sampler2D albedo_tex; // Contains Alpha channel

layout(std140, set = 3, binding = 0) uniform Param
{
    float alpha_cutoff;
};

void main()
{
    float alpha = texture(albedo_tex, out_uv).a;
    if (alpha < alpha_cutoff)
        discard;
}
