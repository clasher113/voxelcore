#ifndef SHADOWS_HLSL_
#define SHADOWS_HLSL_

Texture2D narrowShadowMap : register(t6);
Texture2D wideShadowMap : register(t7);
SamplerComparisonState comparisonSampler : register(s2);

cbuffer ShadowsCBuff : register(b2) {
    float4x4 u_narrowShadowsMatrix;
    float4x4 u_wideShadowsMatrix;
    float u_dayTime;
    int u_shadowsRes;
    float u_shadowsOpacity;
    float u_shadowsSoftness;
}

float calc_shadow(Texture2D shadowsMap, float4x4 shadowMatrix, float4 modelPos, float3 realnormal, float3 normalOffset, float bias) {    
    float step = 1.f / float(u_shadowsRes);
    float4 mpos = mul(shadowMatrix, float4(modelPos.xyz + normalOffset, 1.f));
    float3 projCoords = mpos.xyz / mpos.w;
    projCoords = projCoords * 0.5 + 0.5;
    projCoords.z -= 0.00001 / u_shadowsRes + bias;
    
    float shadow = 0.f;
    if (dot(realnormal, u_sunDir) < 0.f) {
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                float3 offset = float3(x, y, -(abs(x) + abs(y)) * 0.8) * step * 1.f * u_shadowsSoftness;
                float2 pos = projCoords.xy + offset.xy;
                pos.y = 1.f - pos.y;
                shadow += shadowsMap.SampleCmpLevelZero(comparisonSampler, pos, projCoords.z + offset.z);
            }
        }
        
        shadow /= 9.0;
    } else {
        shadow = 0.f;
    }
    
    return shadow;
}

float calc_shadow(float4 modelPos, float3 realnormal, float distance) {
#ifdef ENABLE_SHADOWS
    float s = pow(abs(cos(u_dayTime * PI2)), 0.25f) * u_shadowsOpacity;
    float3 normalOffset = realnormal * (distance > 64.f ? 0.2 : 0.04f);
    
    float shadow = (distance < 80)
        ? calc_shadow(narrowShadowMap, u_narrowShadowsMatrix, modelPos, realnormal, normalOffset, 0.0f)
        : calc_shadow(wideShadowMap, u_wideShadowsMatrix, modelPos, realnormal, normalOffset, 0.001f);
    
    return 0.5f * (1.f + s * shadow);
#else
    return 1.f;
#endif // ENABLE_SHADOWS   
}

#endif // SHADOWS_HLSL_