struct VSInput {
    float3 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    uint light_comp : LIGHT0;
    uint normal_comp : NORMAL0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

cbuffer ShadowsCBuff : register(b0) {
    float4x4 u_model;
    float4x4 u_proj;
    float4x4 u_view;
}

PSInput VShader(VSInput input) {
    PSInput output;
    
    output.texCoord = input.texCoord;
    output.position = mul(u_proj, mul(u_view, mul(u_model, float4(input.position, 1.f))));
    
    return output;
}

Texture2D blocksTexture : register(t0);
SamplerState samplerPointWrap : register(s0);

void PShader(PSInput input) {
    float4 tex_color = blocksTexture.Sample(samplerPointWrap, input.texCoord);
    if (tex_color.a < 0.5) {
        discard;
    }
}