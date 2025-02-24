#ifndef COMMONS_HLSL_
#define COMMONS_HLSL_

#include "constants.hlsl"

float3 apply_planet_curvature(float3 modelPos, float3 pos3d) {
    modelPos.y -= pow(length(pos3d.xz) * CURVATURE_FACTOR, 3.0);
    return modelPos;
}

float4 decompress_light(float compressed_light) {
	float4 result;
	int compressed = asint(compressed_light);
	result.r = ((compressed >> 24) & 0xFF) / 255.f;
	result.g = ((compressed >> 16) & 0xFF) / 255.f;
	result.b = ((compressed >> 8) & 0xFF) / 255.f;
	result.a = (compressed & 0xFF) / 255.f;
	return result;
}

float3 pick_sky_color(TextureCube cubemap, SamplerState _sampler) {
    float3 skyLightColor = cubemap.SampleLevel(_sampler, float3(-0.4f, 0.05f, -0.4f), 0).rgb;
    skyLightColor *= SKY_LIGHT_TINT;
    skyLightColor = min(float3(1.f, 1.f, 1.f), skyLightColor*SKY_LIGHT_MUL);
    skyLightColor = max(MIN_SKY_LIGHT, skyLightColor);
    return skyLightColor;
}

#endif // COMMONS_HLSL_