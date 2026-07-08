struct VSInput {
    float2 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
};

cbuffer UICBuff : register(b0) {
    float4x4 u_projview;
};

PSInput VShader(VSInput input) {
    PSInput output;
    
    output.texCoord = input.texCoord;
    output.color = input.color;
    output.position = mul(u_projview, float4(input.position, 1.f, 1.f));
    
    return output;
}

Texture2D mainTexture : register(t0);
SamplerState samplerPointWrap : register(s0);

float4 PShader(PSInput input) : SV_TARGET {
    float4 color = input.color * mainTexture.Sample(samplerPointWrap, input.texCoord);
    if (color.a == 0.0) 
        discard;
    
    return color;
}