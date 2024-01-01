struct VSInput {
    float2 position : POSITION0;
};

struct PSInput {
    float4 coord : SV_POSITION;
    float3 v_coord : V_COORD;
};

cbuffer CBuff : register(b0) {
    float4x4 c_view;
    float c_ar;
    float c_zoom;
};

PSInput VShader(VSInput input) {
    PSInput output;
    output.v_coord = mul(c_view, float4(input.position * float2(c_ar, 1.f) * c_zoom, -1.f, 1.f)).xyz;
    output.coord = float4(input.position, 0.f, 1.f);
    return output;
}

TextureCube my_texture : register(t0);
SamplerState my_samplerLinear : register(s1);


float4 PShader(PSInput input) : SV_TARGET {
    float3 dir = normalize(input.v_coord);
    return my_texture.Sample(my_samplerLinear, dir);
};
