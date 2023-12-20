struct VSInput
{
    float2 position : POSITION0;
};

struct PSInput
{
    float4 position : SV_POSITION;
};

cbuffer CBuf : register(b0)
{
    float4x4 c_projview;
}

PSInput VShader(VSInput input)
{
    PSInput output;
    output.position = mul(c_projview, float4(input.position, 0.5f, 1.f));
    return output;
}

Texture2D my_texture;
SamplerState my_sampler;

float4 PShader(PSInput input) : SV_TARGET
{
    return float4(1.f, 1.f, 1.f, 1.f);
}