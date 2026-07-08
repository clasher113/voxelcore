#ifndef LIGHTING_HLSL_
#define LIGHTING_HLSL_

float calc_torch_light(float3 normal, float3 modelpos) {
    return max(0.f, 1.f - distance(u_cameraPos, modelpos) / u_torchlightDistance)
        * max(0.f, -dot(normal, normalize(modelpos - u_cameraPos)));
}

float3 calc_torch_light(float3 light, float3 normal, float3 modelPos, float3 torchLightColor, float gamma) {
    float torchlight = calc_torch_light(normal, modelPos);
    return pow(abs(light + mul(torchLightColor, torchlight)), float3(gamma.rrr));
}

float3x3 Inverse(float3x3 m) {
    float Det = determinant(m);
    
    float3x3 adj;
    adj[0][0] = m[1][1] * m[2][2] - m[1][2] * m[2][1];
    adj[0][1] = m[0][2] * m[2][1] - m[0][1] * m[2][2];
    adj[0][2] = m[0][1] * m[1][2] - m[0][2] * m[1][1];
    adj[1][0] = m[1][2] * m[2][0] - m[1][0] * m[2][2];
    adj[1][1] = m[0][0] * m[2][2] - m[0][2] * m[2][0];
    adj[1][2] = m[0][2] * m[1][0] - m[0][0] * m[1][2];
    adj[2][0] = m[1][0] * m[2][1] - m[1][1] * m[2][0];
    adj[2][1] = m[0][1] * m[2][0] - m[0][0] * m[2][1];
    adj[2][2] = m[0][0] * m[1][1] - m[0][1] * m[1][0];
    
    return adj / Det;
}

float3 calc_screen_normal(float3 normal) {
    return mul(normal, Inverse((float3x3)(mul(u_model, u_view))));
}

#endif // LIGHTING_HLSL_