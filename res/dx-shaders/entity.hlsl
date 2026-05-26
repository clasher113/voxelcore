#include "lib/common.hlsl"

struct VSInput {
	float3 position : POSITION0;
	float2 texCoord : TEXCOORD0;
    float3 color : COLOR0;
	float light : LIGHT0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 texCoord : TEXCOORD0;
    float fog : FOG0;
    float3 dir : DIR0;
};

cbuffer CBuff : register(b0) {
    float4x4 u_model;
    float4x4 u_proj;
    float4x4 u_view;
    float3 u_cameraPos;
    float u_gamma;
    float3 u_torchlightColor;
    float u_torchlightDistance;
    float3 u_fogColor;
    float u_fogFactor;
    float u_fogCurve;
    float u_opacity;
    float u_weatherFogOpacity;
    float u_weatherFogDencity;
    float u_weatherFogCurve;
    bool u_alphaClip;
}

Texture2D my_texture : register(t0);
TextureCube my_textureCube : register(t1);
SamplerState my_sampler : register(s0);
SamplerState my_samplerLinear : register(s1);

PSInput VShader(VSInput input) {
    PSInput output;
    
    float4 modelpos = mul(float4(input.position, 1.f), u_model);
    float3 pos3d = modelpos.xyz - u_cameraPos;
    modelpos.xyz = apply_planet_curvature(modelpos.xyz, pos3d);
    
    float4 decomp_light = decompress_light(input.light);
    float3 light = decomp_light.rgb;
    float torchlight = max(0.8f, 1.f - distance(u_cameraPos, modelpos.xyz) / u_torchlightDistance);
    light += torchlight * u_torchlightColor;
    
    output.color = float4(pow(abs(light), float3(u_gamma, u_gamma, u_gamma)), 1.f);
    output.texCoord = input.texCoord;
    output.dir = modelpos.xyz - u_cameraPos;
    
    float3 skyLightColor = pick_sky_color(my_textureCube, my_sampler);
    
    output.color.rgb = max(output.color.rgb, skyLightColor.rgb * decomp_light.a) * input.color;
    output.color.a = u_opacity;
    
    float dist = length(mul(float4(pos3d * FOG_POS_SCALE, 0.f), mul(u_model, u_view)));
    float depth = dist / 256.f;
    output.fog = min(1.0, max(pow(abs(depth * u_fogFactor), u_fogCurve),
                              min(pow(abs(depth * u_weatherFogDencity), u_weatherFogCurve), u_weatherFogOpacity)));
    
    output.position = mul(modelpos, mul(u_view, u_proj));
    
    return output;
}

float4 PShader(PSInput input) : SV_TARGET {
    float3 fogColor = my_textureCube.SampleLevel(my_samplerLinear, input.dir, 0).rgb;
    float4 tex_color = my_texture.Sample(my_sampler, input.texCoord);
    float alpha = input.color.a * tex_color.a;
    
    if (alpha < (u_alphaClip ? 0.5f : 0.15f))
        discard;
    
    return float4(lerp(input.color * tex_color, float4(fogColor, 1.f), input.fog).rgb, alpha);
}