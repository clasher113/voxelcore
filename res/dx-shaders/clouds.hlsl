#include "lib/commons.hlsl"

struct VSInput {
    float3 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    float4 light : LIGHT0_int8_unorm;
    float4 normal : NORMAL0_int8_unorm;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 pos : POS0;
    float2 texCoord : TEXCOORD0;
    float3 dir : DIR0;
    float3 realnormal : REALNORMAL0;
    float3 normal : NORMAL0;
    float4 torchLight : TORCHLIGHT0;
    float3 skyLight : SKYLIGHT0;
    float distance : DISTANCE0;
    float fog : FOG0;
    float emission : EMISSION0;
};

struct PSOutput {
    float4 color : SV_Target0;
    float4 position : SV_Target1;
    float4 normal : SV_Target2;
    float4 emission : SV_Target3;
};

cbuffer CloudsCBuff : register(b0) {
    float4x4 u_model;
    float4x4 u_view;
    float4x4 u_proj;
    float3 u_cameraPos;
    float u_timer;
    float3 u_torchlightColor;
    float u_gamma;
    float3 u_minSkyLight;
    float u_dayTime;
    float4 u_tint;
    float u_torchlightDistance;
}

TextureCube skyboxTexture : register(t1);
SamplerState samplerLinearClamp : register(s3);

#include "lib/lighting.hlsl"
#include "lib/sky.hlsl"
#include "lib/fog.hlsl"

PSInput VShader(VSInput input) {
    PSInput output;
    
    float4 modelpos = mul(u_model, float4(input.position, 1.f));
    modelpos.x += sin(u_timer * 0.05f + modelpos.z * 0.002f) * 1.5f;
    modelpos.z += sin(u_timer * 0.2f + modelpos.x * 0.002f) * 1.5f;
    modelpos.y += sin(u_timer * 0.1f + (modelpos.x + modelpos.z) * 0.001f) * 2.0f;
    
    float3 pos3d = modelpos.xyz - u_cameraPos;
    
    output.realnormal = input.normal.xyz * 2.f - 1.f;
    output.normal = calc_screen_normal(output.realnormal);
    
    output.torchLight = float4(calc_torch_light(
        input.light.rgb, output.realnormal, modelpos.xyz, u_torchlightColor, u_gamma
    ), 1.f);
    output.texCoord = input.texCoord;
    
    output.dir = modelpos.xyz - u_cameraPos;
    float3 skyLightColor = pick_sky_color(skyboxTexture, samplerLinearClamp, u_dayTime, u_minSkyLight);
    output.skyLight = skyLightColor.rgb * input.light.a;
    
    float4x4 viewmodel = mul(u_view, u_model);
    output.distance = length(mul(viewmodel, float4(pos3d, 0.f)));
    
#ifndef ADVANCED_RENDER
    output.fog = calc_fog(length(mul(viewmodel, float4(pos3d * FOG_POS_SCALE, 0.f))) / 256.f);
#else
    output.fog = 0.f;
#endif
    
    output.emission = input.normal.w;
    
    float4 viewmodelpos = mul(u_view, modelpos);
    output.pos = viewmodelpos.xyz;
    output.position = mul(u_proj, viewmodelpos);
    
    return output;
}

PSOutput PShader(PSInput input) {
    PSOutput output;
    
    output.color.rgb = max(input.torchLight.rgb, input.skyLight);
    output.color.a = 1.f;
    output.color *= u_tint;
    
#ifndef ADVANCED_RENDER
    float3 fogColor = skyboxTexture.SampleLevel(samplerLinearClamp, input.dir, 0).rgb;
    output.color = lerp(output.color, float4(fogColor, 1.f), input.fog);
#endif
    output.position = float4(input.pos, 1.f);
    output.normal = float4(input.normal, 1.f);
    output.emission = float4(input.emission.rrr, 1.f);
    
    return output;
}