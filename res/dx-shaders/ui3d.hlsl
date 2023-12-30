struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};

cbuffer CBuff : register(b0) {
    float4x4 c_projview;
    float4x4 c_apply;
}

PSInput VShader(VSInput input) {
    PSInput output;
    output.texCoord = input.texCoord;
    output.color = input.color;
    output.position = mul(float4(input.position, 1.f), mul(c_projview, c_apply));
    return output;
}

Texture2D my_texture;
SamplerState my_sampler;

float4 PShader(PSInput input) : SV_TARGET {
    return input.color * my_texture.Sample(my_sampler, input.texCoord);
}