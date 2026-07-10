#ifndef COMMONS_SKY_
#define COMMONS_SKY_

#include "constants.hlsl"

float3 pick_sky_color(TextureCube cubemap, SamplerState _sampler) {
    float3 skyLightColor = cubemap.SampleLevel(_sampler, float3(0.4f, 0.05f, 0.4f), 0).rgb;
    skyLightColor *= SKY_LIGHT_TINT;
    skyLightColor = min(1.f.rrr, skyLightColor * SKY_LIGHT_MUL);
    skyLightColor = max(MIN_SKY_LIGHT, skyLightColor);
    return skyLightColor;
}

#endif // COMMONS_SKY_