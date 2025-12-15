#ifndef _RESERVOIR_GLSL_
#define _RESERVOIR_GLSL_

#extension GL_GOOGLE_include_directive : enable
#include "../common/oct.glsl"

precision highp float;

struct Reservoir
{
    /* Hit information */

    vec3 hit_position;
    vec3 hit_normal;
    vec3 hit_luminance;
    vec3 start_position;
    vec3 start_normal;

    /* Reservoir data */

    float w;
    float M;
    float W;
};

Reservoir decode_reservoir(vec4 tex1, uvec4 tex2, uvec4 tex3, vec4 tex4)
{
    Reservoir reservoir;

    reservoir.w = tex1.x;
    reservoir.M = tex1.y;
    reservoir.W = tex1.z;

    if (isnan(reservoir.w) || isinf(reservoir.w)) reservoir.w = 0.0;
    if (isnan(reservoir.M) || isinf(reservoir.M)) reservoir.M = 0.0;
    if (isnan(reservoir.W) || isinf(reservoir.W)) reservoir.W = 0.0;

    reservoir.hit_normal = octToNormal(unpackSnorm2x16(tex2.w));
    reservoir.hit_position = vec3(
            uintBitsToFloat(tex2.x),
            uintBitsToFloat(tex2.y),
            uintBitsToFloat(tex2.z)
        );

    reservoir.start_normal = octToNormal(unpackSnorm2x16(tex3.w));
    reservoir.start_position = vec3(
            uintBitsToFloat(tex3.x),
            uintBitsToFloat(tex3.y),
            uintBitsToFloat(tex3.z)
        );

    reservoir.hit_luminance = tex4.rgb;

    return reservoir;
}

void encode_reservoir(Reservoir reservoir, out vec4 tex1, out uvec4 tex2, out uvec4 tex3, out vec4 tex4)
{
    tex1 = vec4(reservoir.w, reservoir.M, reservoir.W, 0.0);

    tex2 = uvec4(
            floatBitsToUint(reservoir.hit_position.x),
            floatBitsToUint(reservoir.hit_position.y),
            floatBitsToUint(reservoir.hit_position.z),
            packSnorm2x16(normalToOct(reservoir.hit_normal))
        );

    tex3 = uvec4(
            floatBitsToUint(reservoir.start_position.x),
            floatBitsToUint(reservoir.start_position.y),
            floatBitsToUint(reservoir.start_position.z),
            packSnorm2x16(normalToOct(reservoir.start_normal))
        );

    tex4 = vec4(reservoir.hit_luminance, 0.0);
}

void update_reservoir(
    inout Reservoir reservoir,
    float p_hat,
    float p,
    vec3 hit_position,
    vec3 hit_normal,
    vec3 start_position,
    vec3 start_normal,
    vec3 luminance,
    vec4 noise_unorm
)
{
    float weight = p_hat / p;

    reservoir.M += 1.0;
    reservoir.w += weight;

    float W = reservoir.w / (reservoir.M * p_hat);

    if (reservoir.w > 0 && noise_unorm.z < weight / reservoir.w)
    {
        reservoir.hit_luminance = luminance;
        reservoir.hit_position = hit_position;
        reservoir.hit_normal = hit_normal;
        reservoir.start_position = start_position;
        reservoir.start_normal = start_normal;
        reservoir.W = W;
    }
}

void clamp_reservoir(inout Reservoir reservoir, float M_max)
{
    if (M_max <= 0.0) return;
    if (reservoir.M > M_max)
    {
        float scale = M_max / reservoir.M;
        reservoir.w *= scale;
        reservoir.M = M_max;

        // sanitize numeric issues
        if (isnan(reservoir.w) || isinf(reservoir.w)) reservoir.w = 0.0;
        if (isnan(reservoir.M) || isinf(reservoir.M)) reservoir.M = M_max;
        // reservoir.W remains valid because scaling preserves W = w/(M*p_hat)
    }
}

#endif
