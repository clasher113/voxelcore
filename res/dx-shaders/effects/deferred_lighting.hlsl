#include "../lib/fog.hlsl"
#include "../lib/constants.hlsl"
#include "../lib/shadows.hlsl"

float4 effect(PSInput input) {
    float4 pos = positionTexture.Sample(samplerPointWrap, input.uv);
    float light = 1.f;
    
#ifdef ENABLE_SSAO
    light = 0.f;
    float z = pos.z;
    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            float2 offset = float2(x, y) / u_screenSize;
            light += ssaoTexture.Sample(samplerPointWrap, input.uv + offset * 2.f).r;
        }
    }
    light /= 24.f;
#endif // ENABLE_SSAO

    float4 modelpos = mul(u_inverseView, pos);
    float3 normal = mul(normalTexture.Sample(samplerPointWrap, input.uv).xyz, (float3x3)(u_view));
    float3 dir = modelpos.xyz - u_cameraPos;
    
    float emission = emissionTexture.Sample(samplerPointWrap, input.uv).r;
    
#ifdef ENABLE_SHADOWS
    light *= calc_shadow(modelpos, normal, length(pos));
#endif // ENABLE_SHADOWS
    
    light = max(light, emission);
    
    light = pow(abs(light), u_gamma);
    
    float3 fogColor = skyboxTexture.SampleLevel(samplerLinearWrap, dir, 0).rgb;
    float fog = calc_fog(length(mul(u_view, float4((modelpos.xyz - u_cameraPos) * FOG_POS_SCALE, 0.f))) / 256.f);
    return float4(lerp(screenTexture.Sample(samplerPointWrap, input.uv).rgb * lerp(1.f, light, 1.f), fogColor, fog), 1.f);
}