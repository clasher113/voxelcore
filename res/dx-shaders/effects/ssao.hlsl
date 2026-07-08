cbuffer Params : register(b1) {
   int u_kernelSize = 16;
   float u_radius = 0.4;
   float u_bias = 0.006;
   float3 u_ssaoSamples[64]; 
}

float4 effect(PSInput input) {
    float2 noiseScale = u_screenSize / 4.f;
    
    float3 position = positionTexture.Sample(samplerPointClamp, input.uv).xyz;
    float3 normal = normalTexture.Sample(samplerPointClamp, input.uv).xyz;
    float3 randomVec = normalize(ssaoTexture.Sample(samplerPointWrap, input.uv * noiseScale).xyz);
    
    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 tbn = float3x3(tangent, bitangent, normal);
    
    float occlusion = 0.f;
    for (int i = 0; i < u_kernelSize; i++)
    {
        float3 samplePos = mul(u_ssaoSamples[i], tbn);
        samplePos = position + samplePos * u_radius;
        
        float4 offset = float4(samplePos, 1.f);
        offset = mul(u_projection, offset); 
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5f + 0.5f.rr;
        offset.y = 1.f - offset.y;
        
        float sampleDepth = positionTexture.Sample(samplerPointClamp, offset.xy).z;
        float rangeCheck = smoothstep(0.f, 1.f, u_radius / abs(position.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + u_bias ? 1.f : 0.f) * rangeCheck;
    }
    occlusion = min(1.f, 1.05f - (occlusion / u_kernelSize));
    occlusion = max(occlusion, emissionTexture.Sample(samplerPointClamp, input.uv).r);
    
    float z = -position.z * 0.01f;
    z = max(0.f, 1.f - z);
    return float4(lerp(1.f, occlusion, z), 0.f, 0.f, 1.f);
}