struct VSInput {
    float2 position : POSITION0;
};

struct PSInput {
    float4 coord : SV_POSITION;
    float3 v_coord : V_COORD;
};

struct PSOutput {
    float4 color : SV_Target0;
    float4 position : SV_Target1;
    float4 normal : SV_Target2;
};

cbuffer BackgroundCBuff : register(b0) {
    float4x4 u_view;
    float u_ar;
    float u_zoom;
};

PSInput VShader(VSInput input) {
    PSInput output;
    
    output.v_coord = mul(float4(input.position * float2(u_ar, 1.f) * u_zoom, -1.f, 1.f), u_view).xyz;
    output.coord = float4(input.position, 1.f - 1e-6, 1.f);
    
    return output;
}

TextureCube skyboxTexture : register(t1);
SamplerState samplerLinearClamp : register(s3);

PSOutput PShader(PSInput input) {
    PSOutput output;
    
    float3 dir = normalize(input.v_coord) * 1e6;
    output.position = mul(float4(dir, 1.f), u_view);
    output.normal = float4(0.f, 0.f, 1.f, 1.f);
    output.color = skyboxTexture.SampleLevel(samplerLinearClamp, dir, 0.f);

    return output;
};
