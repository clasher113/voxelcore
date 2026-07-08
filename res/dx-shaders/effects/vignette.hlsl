cbuffer Params : register(b1) {
   float p_radius = 1.1f;
   float p_softness = 0.7f;
}

float4 apply_vignette(float4 color, float2 uv) {
    float2 position = uv - 0.5f.rr;
    float dist = length(position);
    float vignette = smoothstep(p_radius, p_radius - p_softness, dist);
    return float4(color.rgb * vignette, 1.0);
}

float4 effect(PSInput input) {
    return apply_vignette(screenTexture.Sample(samplerPointWrap, input.uv), input.uv);
}
