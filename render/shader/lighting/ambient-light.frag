#version 460

#extension GL_GOOGLE_include_directive : enable

#include "../common/gbuffer-storage.glsl"

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 ndc;

layout(location = 0) out vec4 out_light_buffer;

layout(set = 2, binding = 0) uniform sampler2D albedo_tex;
layout(set = 2, binding = 1) uniform usampler2D lighting_info_tex;
layout(set = 2, binding = 2) uniform sampler2D ao_tex;

layout(std140, set = 3, binding = 0) uniform Param
{
    vec3 ambient_intensity; // Intensity of RGB
    float ao_intensity; // Intensity of AO
};

void main()
{
    vec4 albedo_tex_sample = textureLod(albedo_tex, uv, 0);

    uvec2 lighting_info_tex_sample = textureLod(lighting_info_tex, uv, 0).rg;
    float ao_tex_sample = textureLod(ao_tex, uv, 0.0).r;

    GBufferLighting lighting = unpack_gbuffer_lighting(lighting_info_tex_sample);

    float ao_factor = clamp(mix(1.0, lighting.occlusion * (1.0 - ao_tex_sample), ao_intensity), 0.0, 1.0);

    out_light_buffer = vec4(
            albedo_tex_sample.rgb * ambient_intensity * ao_factor,
            0.0
        );
}
