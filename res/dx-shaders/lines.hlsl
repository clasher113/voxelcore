struct VSInput {
    float3 position : POSITION0;
    float4 color : COLOR0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

cbuffer CBuf : register(b0) {
    float4x4 c_projview;
}

PSInput VShader(VSInput input) {
    PSInput output;
    output.color = input.color;
    output.position = mul(float4(input.position, 1.f), c_projview);
    return output;
}

float4 PShader(PSInput input) : SV_TARGET {
    return input.color;
}