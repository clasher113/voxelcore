#include "lib/commons.hlsl"

struct VSInput {
    float3 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    float4 light : LIGHT0_int8_unorm;
    float4 normal : NORMAL0_int8_unorm;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 torchlight : TORCHLIGHT0;
    float4 modelpos : MODELPOS0;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float distance : DISTANCE0;
    float3 realnormal : REALNORMAL0;
    float fog : FOG0;
    float3 dir : DIR0;
    float emission : EMISSION0;
    float3 skyLight : SKYLIGHT0;
    float3 pos : POS0;
};

cbuffer TranslucentCBuff : register(b0) {
    float4x4 u_model;
    float4x4 u_proj;
    float4x4 u_view;
    float3 u_cameraPos;
    float3 u_torchlightColor;
    float u_torchlightDistance;
    float3 u_sunDir;
    float u_gamma;
    float3 u_minSkyLight;
    float u_dayTime;
    bool u_alphaClip;
    bool u_debugLights;
    bool u_debugNormals;
}

Texture2D blocksTexture : register(t0);
TextureCube skyboxTexture : register(t1);
SamplerState samplerPointWrap : register(s0);
SamplerState samplerLinearClamp : register(s3);

#include "lib/lighting.hlsl"
#include "lib/fog.hlsl"
#include "lib/sky.hlsl"

PSInput VShader(VSInput input) {
    PSInput output;
    
    output.modelpos = mul(u_model, float4(input.position, 1.f));
    float3 pos3d = output.modelpos.xyz - u_cameraPos;
        
    output.realnormal = mul(2.f, input.normal.xyz) - 1.f;
    output.normal = calc_screen_normal(output.realnormal);
        
    output.torchlight = float4(calc_torch_light(
        input.light.rgb, output.realnormal, output.modelpos.xyz, u_torchlightColor, u_gamma
    ), 1.f);
    output.texCoord = input.texCoord;
    
    output.dir = output.modelpos.xyz - u_cameraPos;
    float3 skyLightColor = pick_sky_color(skyboxTexture, samplerLinearClamp, u_dayTime, u_minSkyLight);
    output.skyLight = skyLightColor.rgb * input.light.a;
    
    float4x4 viewmodel = mul(u_view, u_model);
    output.distance = length(mul(viewmodel, float4(pos3d, 0.f)));
    output.fog = calc_fog(length(mul(viewmodel, float4(pos3d * FOG_POS_SCALE, 0.f))) / 256.f);
    output.emission = input.normal.w;
    
    float4 viewModelPos = mul(u_view, output.modelpos);
    output.pos = viewModelPos.xyz;
    output.position = mul(u_proj, viewModelPos);
    
    return output;
}

#include "lib/shadows.hlsl"

float4 PShader(PSInput input) : SV_TARGET {
    float4 texColor = blocksTexture.Sample(samplerPointWrap, input.texCoord);
    float alpha = texColor.a;
    if (u_alphaClip) {
        if (alpha < 0.2f)
            discard;
        alpha = 1.f;
    }
    else {
        if (alpha < 0.002f)
            discard;
    }
    if (u_debugLights)
        texColor.rgb = u_debugNormals ? (mul(0.5f, input.normal) + 0.5f) : float3(1.f, 1.f, 1.f);

    float4 color = texColor;
    color.rgb *= min(1.f.rrr, input.torchlight.rgb + input.skyLight
        * calc_shadow(input.modelpos, input.realnormal, length(input.position)));
    
    float3 fogColor = skyboxTexture.SampleLevel(samplerLinearClamp, input.dir, 0.f).rgb;
    color = lerp(color, float4(fogColor, 1.f), input.fog);
    color.a = alpha;
    
    return color;
}