struct VSInput {
    float2 position : POSITION;
};

struct PSInput {
    float4 coord : SV_POSITION;
};

cbuffer CBuf : register(b0) {
    float4x4 c_view;
    float c_ar;
    float c_zoom;
};

PSInput VShader(VSInput input) {
    PSInput output;
    output.coord = mul(c_view, float4(input.position * float2(c_ar, 1.0f) * c_zoom, -1.0, 1.0));
    return output;
}

TextureCube my_texture;
samplerCUBE my_sampler;

float4 PShader(PSInput input) : SV_TARGET {
    float3 dir = normalize(input.coord);
    return float4(1.f, 1.f, 1.f, 1.f); 
    //my_texture.Sample(my_sampler, dir);
};
