// Computes HBAO

#version 460

#extension GL_GOOGLE_include_directive : enable

#include "../common/gbuffer-storage.glsl"
#include "../common/ndc-uv-conv.glsl"

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 ndc;

layout(location = 0) out float out_ao; // AO Intensity Output, 0 for no occlusion, 1 for fully occluded

layout(set = 2, binding = 0) uniform sampler2D depth_tex; // Depth Input (Current Frame)
layout(set = 2, binding = 1) uniform usampler2D light_info_tex; // Light Info Input (Current Frame) -- for normals
layout(set = 2, binding = 2) uniform sampler2D prev_depth_tex; // Depth Input (Previous Frame)
layout(set = 2, binding = 3) uniform sampler2D prev_ao_tex; // AO Input (Previous Frame)

layout(std140, set = 3, binding = 0) uniform Params
{
    mat4 camera_mat_inv;
    mat4 view_mat;
    mat4 proj_mat_inv;
    mat4 prev_camera_mat;
    float random_seed;
    float radius;
    float blend_alpha;
};

const uint SAMPLE_COUNT = 3;
const uint RAY_COUNT = 4;
const float PI = 3.14159265359;

ivec2 fragcoord = ivec2(floor(gl_FragCoord.xy));
ivec2 ao_render_size = textureSize(prev_ao_tex, 0);
ivec2 depth_render_size = textureSize(depth_tex, 0);

float noise_function(in vec2 xy, in float seed)
{
    const float PHI = 1.61803398874989484820459;
    return fract(tan(distance(xy * PHI, xy.yx) * seed) * xy.x);
}

vec3 homo_transform(mat4 mat, vec3 vec)
{
    const vec4 homo = mat * vec4(vec, 1.0);
    return homo.xyz / homo.w;
}

vec3 get_view_normal()
{
    const vec2 little_offset = 0.5 / vec2(depth_render_size);
    vec3 world_normal = unpack_normal(textureLod(light_info_tex, uv - little_offset, 0).r);
    vec4 view_normal = view_mat * vec4(world_normal, 0.0);
    return view_normal.xyz;
}

void main()
{
    const float depth = textureLod(depth_tex, uv, 0).r;
    if (depth == 0.0) discard;

    const vec3 world_pos = homo_transform(camera_mat_inv, vec3(ndc, depth));
    const vec3 view_pos = homo_transform(proj_mat_inv, vec3(ndc, depth));
    const vec3 view_normal = get_view_normal();

    const vec3 offseted_world_pos = homo_transform(
            camera_mat_inv,
            vec3(ndc + vec2(radius) / vec2(depth_render_size), depth)
        );
    const float clamp_distance = distance(world_pos, offseted_world_pos);

    float occlusion_sum = 0.0;

    for (float phi = 0.0; phi < 2.0 * PI + 0.001; phi += 2.0 * PI / RAY_COUNT)
    {
        const float jittered_phi = phi + noise_function(uv * vec2(156.213, 481.53), random_seed + phi);
        const vec2 sample_direction = vec2(cos(jittered_phi), sin(jittered_phi));
        const vec2 sample_step = sample_direction * radius / depth_render_size / SAMPLE_COUNT;

        vec2 sample_ndc = ndc;
        float max_cos_angle = 0;
        uint max_angle_idx = SAMPLE_COUNT;

        for (uint i = 0; i < SAMPLE_COUNT; i++)
        {
            sample_ndc += sample_step * mix(0.5, 1.5, noise_function(sample_ndc * vec2(152.12, 843.2), random_seed));
            
            if (any(greaterThan(abs(sample_ndc), vec2(1.0)))) break;

            const float sample_depth = textureLod(depth_tex, ndc_to_uv(sample_ndc), 0.0).r;
            if (sample_depth == 0.0) continue;

            const vec3 sample_view_pos = homo_transform(proj_mat_inv, vec3(sample_ndc, sample_depth));
            const vec3 sample_ray_dir = sample_view_pos - view_pos;
            float cos_angle = dot(normalize(sample_ray_dir), view_normal);

            if (sample_ray_dir.z > clamp_distance) continue;

            if (cos_angle > max_cos_angle)
            {
                max_cos_angle = cos_angle;
                max_angle_idx = i;
            }
        }

        const float attenuation = float(SAMPLE_COUNT - max_angle_idx) / SAMPLE_COUNT;
        const float sample_occlusion = max(mix(-0.5, 1.0, max_cos_angle), 0.0); // Clamp at 30deg

        occlusion_sum += sample_occlusion * attenuation;
    }

    occlusion_sum /= float(RAY_COUNT);

    vec4 prev_homo = prev_camera_mat * vec4(world_pos, 1.0);
    if (prev_homo.w > 0.0)
    {
        vec3 prev_ndc = prev_homo.xyz / prev_homo.w;
        if (all(lessThanEqual(abs(prev_ndc.xy), vec2(1.0))))
        {
            vec2 prev_uv = ndc_to_uv(prev_ndc.xy);
            float prev_depth = textureLod(prev_depth_tex, prev_uv, 0.0).r;
            if (distance(prev_depth, prev_ndc.z) / prev_depth < 0.00001)
            {
                float prev_ao = textureLod(prev_ao_tex, prev_uv, 0.0).r;
                occlusion_sum = mix(prev_ao, occlusion_sum, blend_alpha);
            }
        }
    }

    out_ao = occlusion_sum;
}
