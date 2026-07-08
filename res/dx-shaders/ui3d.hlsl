struct VSInput {
    float3 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;
};

cbuffer UI3DCBuff : register(b0) {
    float4x4 u_projview;
    float4x4 u_apply;
}

PSInput VShader(VSInput input) {
    PSInput output;
    
    output.texCoord = input.texCoord;
    output.color = input.color;
    output.position = mul(u_apply, mul(u_projview, float4(input.position, 1.f)));
    
    return output;
}

Texture2D mainTexture : register(t0);
SamplerState samplerPointWrap : register(s0);

float4 PShader(PSInput input) : SV_TARGET {
    float4 outColor = input.color * mainTexture.Sample(samplerPointWrap, input.texCoord);
    if (outColor.a == 0.f)
        discard;
    
    return outColor;
}