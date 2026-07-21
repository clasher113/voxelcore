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
    float3 dir : DIRECTION0;
    float fog : FOG0;
    float3 normal : NORMAL0;
    float3 realNormal : REALNORMAL0;
    float4 torchLight : TORCHLIGHT0;
    float3 skyLight : SKYLIGHT0;
    float distance : DISTANCE0;
    float emission : EMISSION0;
};

struct PSOutput {
    float4 color : SV_Target0;
    float4 position : SV_Target1;
    float4 normal : SV_Target2;
    float4 emission : SV_Target3;
};

cbuffer MainCBuff : register(b0) {
    float4x4 u_model;
    float4x4 u_proj;
    float4x4 u_view;
    float3 u_cameraPos;
    float u_gamma;
    float3 u_torchlightColor;
    float u_torchlightDistance;
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
	
    float4 modelpos = mul(u_model, float4(input.position, 1.f));
    float3 pos3d = modelpos.xyz - u_cameraPos;
        
    output.realNormal = input.normal.xyz * 2.f - 1.f;
    output.normal = calc_screen_normal(output.realNormal);
        
    output.torchLight = float4(calc_torch_light(
        input.light.rgb, output.realNormal, modelpos.xyz, u_torchlightColor, u_gamma
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
    
    float4 texColor = blocksTexture.Sample(samplerPointWrap, input.texCoord);
    float alpha = texColor.a;
    if (u_alphaClip) {
        if (alpha < 0.2f)
            discard;
        alpha = 1.f;
    } else {
        if (alpha < 0.002f)
            discard;
    }
    if (u_debugLights) {
        texColor.rgb = u_debugNormals ? (input.normal * 0.5f + 0.5f) : 1.f.rrr;
    }
    
    output.color = texColor;
    output.color.rgb *= max(input.torchLight.rgb, input.skyLight);
    
#ifndef ADVANCED_RENDER
    float3 fogColor = skyboxTexture.SampleLevel(samplerLinearClamp, input.dir, 0).rgb;
    output.color = lerp(output.color, float4(fogColor, 1.f), input.fog);
#endif
    output.color.a = alpha;
    output.position = float4(input.pos, 1.f);
    output.normal = float4(input.normal, 1.f);
    output.emission = float4(input.emission.xxx, 1.f);
    
    return output;
}