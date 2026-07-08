float4 effect(PSInput input) {
    float4 color = screenTexture.Sample(samplerPointWrap, input.uv);
    color = lerp(color, 1.0 - color, u_intensity);
    color.a = 1.0;
    return color;
}
