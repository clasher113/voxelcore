struct VSInput {
    float3 position : POSITION0;
    float4 color : COLOR0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

cbuffer LinesCBuff : register(b0) {
    float4x4 u_projview;
}

PSInput VShader(VSInput input) {
    PSInput output;
    
    output.color = input.color;
    output.position = mul(u_projview, float4(input.position, 1.f));

    return output;
}

cbuffer ThickLineCBuff : register(b1) {
    float2 c_viewportSize;
    float c_lineWidth;
}

[maxvertexcount(4)]
void GShader(line PSInput input[2], inout TriangleStream<PSInput> triStream) {
    float2 p0 = input[0].position.xy / input[0].position.w;
    float2 p1 = input[1].position.xy / input[1].position.w;

    float2 p0_screen = (p0 + 1.f) * 0.5f * c_viewportSize;
    float2 p1_screen = (p1 + 1.f) * 0.5f * c_viewportSize;

    float2 lineDir = p1_screen - p0_screen;
    float2 lineNormal = normalize(float2(-lineDir.y, lineDir.x));

    float2 pixelOffset = lineNormal * (c_lineWidth * 0.5f);

    float2 clipOffset0 = (pixelOffset / c_viewportSize) * 2.f * input[0].position.w;
    float2 clipOffset1 = (pixelOffset / c_viewportSize) * 2.f * input[1].position.w;

    PSInput output;

    output.position = float4(input[0].position.xy + clipOffset0, input[0].position.zw);
    output.color = input[0].color;
    triStream.Append(output);
    
    output.position = float4(input[0].position.xy - clipOffset0, input[0].position.zw);
    output.color = input[0].color;
    triStream.Append(output);

    output.position = float4(input[1].position.xy + clipOffset1, input[1].position.zw);
    output.color = input[1].color;
    triStream.Append(output);

    output.position = float4(input[1].position.xy - clipOffset1, input[1].position.zw);
    output.color = input[1].color;
    triStream.Append(output);

    triStream.RestartStrip();
}

float4 PShader(PSInput input) : SV_TARGET {
    return input.color;
}