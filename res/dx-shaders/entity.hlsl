#include "lib/commons.hlsl"

struct VSInput {
	float3 position : POSITION0;
	float2 texCoord : TEXCOORD0;
    float3 color : COLOR0;
	float4 light : LIGHT0_int8_unorm;
    float4 normal : NORMAL0_int8_unorm;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 pos : POS0;
    float4 color : COLOR0;
    float2 texCoord : TEXCOORD0;
    float fog : FOG0;
    float3 dir : DIR0;
    float3 normal : NORMAL0;
    float3 realNormal : REALNORMAL0;
    float distance : DISTANCE0;
    float emission : EMISSION0;
};

struct PSOutput {
    float4 color : SV_Target0;
    float4 position : SV_Target1;
    float4 normal : SV_Target2;
    float4 emission : SV_Target3;
};

cbuffer EntityCBuff : register(b0) {
    float4x4 u_model;
    float4x4 u_proj;
    float4x4 u_view;
    float3 u_cameraPos;
    float u_gamma;
    float3 u_torchlightColor;
    float u_torchlightDistance;
    float3 u_minSkyLight;
    float u_dayTime;
    float u_opacity;
    bool u_alphaClip;
}

Texture2D entityTexture : register(t0);
TextureCube skyboxTexture : register(t1);
SamplerState samplerPointWrap : register(s0);
SamplerState samplerLinearClamp : register(s3);

#include "lib/sky.hlsl"
#include "lib/lighting.hlsl"
#include "lib/fog.hlsl"

PSInput VShader(VSInput input) {
    PSInput output;
    
    float4 modelpos = mul(u_model, float4(input.position, 1.f));
    float3 pos3d = modelpos.xyz - u_cameraPos;
    
    output.realNormal = input.normal.xyz * 2.f - 1.f;
    output.normal = calc_screen_normal(output.realNormal);
    
    output.color = float4(calc_torch_light(
        input.light.rgb, output.realNormal, modelpos.xyz, u_torchlightColor, u_gamma
    ), 1.f);
    output.texCoord = input.texCoord;
    
    output.dir = modelpos.xyz - u_cameraPos;
    float3 skyLightColor = pick_sky_color(skyboxTexture, samplerLinearClamp, u_dayTime, u_minSkyLight);    
    output.color.rgb = max(output.color.rgb, skyLightColor.rgb * input.light.a) * input.color;
    output.color.a = u_opacity;
    
    float4x4 viewModel = u_view * u_model;
    output.distance = length(mul(float4(pos3d, 0.f), viewModel));
    output.fog = calc_fog(length(mul(viewModel, float4(pos3d * FOG_POS_SCALE, 0.f))) / 256.f);
    output.emission = input.normal.w;
    
    float4 viewModelPos = mul(u_view, modelpos);
    output.pos = viewModelPos.xyz;
    output.position = mul(u_proj, viewModelPos);
    
    return output;
}

PSOutput PShader(PSInput input) {
    PSOutput output;
    
    float4 texColor = entityTexture.Sample(samplerPointWrap, input.texCoord);
    float alpha = input.color.a * texColor.a;
    
    if (alpha < (u_alphaClip ? 0.5f : 0.15f)) {
        discard;
    }
    output.color = input.color * texColor;
    
#ifndef ADVANCED_RENDER
    float3 fogColor = skyboxTexture.SampleLevel(samplerLinearClamp, input.dir, 0).rgb;
    output.color = lerp(output.color, float4(fogColor, 1.f), input.fog);
#endif

    output.color.a = alpha;
    output.position = float4(input.pos, 1.f);
    output.normal = float4(input.normal, 1.f);
    output.emission = float4(input.emission.rrr, 1.f);
    
    return output;
}