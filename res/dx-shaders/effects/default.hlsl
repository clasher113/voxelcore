float4 effect(PSInput input) {
    return screenTexture.Sample(samplerPointWrap, input.uv);
}