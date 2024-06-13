struct VSInput {
    float2 position : POSITION0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

cbuffer CBuff : register(b0) {
    int2 u_screenSize;
    float u_timer;
    float u_dayTime;
};

PSInput VShader(VSInput input) {
    PSInput output;
    float2 a = float2(u_screenSize.x + u_timer * 0.000000001, u_screenSize.y + u_dayTime * 0.000000001) * 0.000000001;
    output.texCoord = input.position * 0.5f + 0.5f + a;
    output.texCoord.y = 1 - output.texCoord.y;
    output.position = float4(input.position, 0.f, 1.f);
    return output;
}

Texture2D my_texture : register(t0);
SamplerState my_sampler : register(s0);

float4 PShader(PSInput input) : SV_TARGET {
    float2 a = float2(u_screenSize.x + u_timer, u_screenSize.y + u_dayTime);
    return my_texture.Sample(my_sampler, input.texCoord);
}