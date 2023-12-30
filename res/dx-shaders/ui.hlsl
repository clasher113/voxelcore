struct VSInput {
    float2 position : POSITION;
    float2 texCoord : UV;
    float4 color : COLOR;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
};

cbuffer CBuff : register(b0) {
    float4x4 c_projview;
};

PSInput VShader(VSInput input) {
    PSInput output;
    output.texCoord = input.texCoord;
    output.color = input.color;
    output.position = mul(float4(input.position, 1.f, 1.f), c_projview);
    return output;
}

Texture2D my_texture : register(t0);
SamplerState my_sampler : register(s0);

float4 PShader(PSInput input) : SV_TARGET {
    return input.color * my_texture.Sample(my_sampler, input.texCoord);
}