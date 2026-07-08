struct VSInput {
    float2 position : POSITION0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : UV0;
};

cbuffer EffectCBuff : register(b0) {
    int2 u_screenSize;
    float u_intensity;
    float u_timer;
    float4x4 u_projection;
    float4x4 u_view;
    float4x4 u_inverseView;
    float3 u_sunDir;
    float u_gamma;
    float3 u_cameraPos;
    bool u_enableShadows;
}

PSInput VShader(VSInput input) {
    PSInput output;
    
    output.uv = input.position * 0.5 + 0.5;
    output.uv.y = 1 - output.uv.y;
    output.position = float4(input.position, 0.f, 1.f);
    
    return output;
}

SamplerState samplerPointWrap : register(s0);
SamplerState samplerLinearWrap : register(s1);
SamplerState samplerLinearClamp : register(s3);
SamplerState samplerPointClamp : register(s4);

Texture2D screenTexture : register(t0);
TextureCube skyboxTexture : register(t1);
Texture2D positionTexture : register(t2);
Texture2D normalTexture : register(t3);
Texture2D emissionTexture : register(t4);
Texture2D ssaoTexture : register(t5);

#include "__effect__"

float4 PShader(PSInput input) : SV_TARGET {
    return effect(input);
}