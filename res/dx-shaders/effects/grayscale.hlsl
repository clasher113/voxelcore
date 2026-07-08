float4 effect(PSInput input) {
    float3 color = screenTexture.Sample(samplerPointWrap, input.uv).rgb;
    float m = (color.r + color.g + color.b) / 3.0;
    return float4(lerp(color, m.rrr, u_intensity), 1.0);
}
