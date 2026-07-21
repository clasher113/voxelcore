#ifndef COMMONS_SKY_
#define COMMONS_SKY_

#include "constants.hlsl"

float3 pick_sky_color(TextureCube cubemap, SamplerState _sampler, float dayTime, float3 minSkyLight) {
    float elevation = sin(dayTime * PI2) * 0.2f;
    float3 skyLightColor = cubemap.SampleLevel(_sampler, float3(0.f, elevation, -0.4f), 0).rgb;
    skyLightColor *= SKY_LIGHT_TINT;
    skyLightColor = min(1.f.rrr, skyLightColor * SKY_LIGHT_MUL);
    skyLightColor = max(minSkyLight, skyLightColor);
    return skyLightColor;
}

#endif // COMMONS_SKY_