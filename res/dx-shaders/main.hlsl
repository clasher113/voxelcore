struct VSInput {
	float3 position : POSITION0;
	float2 texCoord : TEXCOORD0;
	float light : LIGHT0;
};

struct PSInput {
	float4 position : SV_POSITION;
	float4 color : COLOR0;
	float2 texCoord : TEXCOORD0;
	float3 dir : DIRECTION0;
	float distance : DISTANCE0;
};

cbuffer CBuff : register(b0) {
	float4x4 u_model;
	float4x4 u_proj;
	float4x4 u_view;
	float3 u_skyLightColor;
	float u_gamma;
	float3 u_cameraPos;
    float u_torchlightDistance;
    float3 u_torchlightColor;
	float u_fogFactor;
	float3 u_fogColor;
	float u_fogCurve;
}

float4 decompress_light(float compressed_light) {
	float4 result;
	int compressed = asint(compressed_light);
	result.r = ((compressed >> 24) & 0xFF) / 255.f;
	result.g = ((compressed >> 16) & 0xFF) / 255.f;
	result.b = ((compressed >> 8) & 0xFF) / 255.f;
	result.a = (compressed & 0xFF) / 255.f;
	return result;
}

Texture2D my_texture : register(t0);
TextureCube my_textureCube : register(t1);
SamplerState my_sampler : register(s0);
SamplerState my_samplerLinear : register(s1);

#define SKY_LIGHT_MUL 2.5

PSInput VShader(VSInput input) {
	PSInput output;
    float4 modelpos = mul(float4(input.position, 1.f), u_model);
	float2 pos2d = modelpos.xz - u_cameraPos.xz;
    float4 viewmodelpos = mul(modelpos, u_view);
	float4 decomp_light = decompress_light(input.light);
	float3 light = decomp_light.rgb;
    float torchlight = max(0.f, 1.f - distance(u_cameraPos, modelpos.xyz) / u_torchlightDistance);
	output.dir = modelpos.xyz - u_cameraPos;
	light += torchlight * u_torchlightColor;
    output.color = float4(pow(abs(light), float3(u_gamma, u_gamma, u_gamma)), 1.f);
    output.texCoord = input.texCoord;
	
    float3 skyLightColor = my_textureCube.SampleLevel(my_sampler, float3(-0.4f, 0.05f, -0.4f), 0).rgb;
	skyLightColor.g *= 0.9;
	skyLightColor.b *= 0.8;
    skyLightColor = min(float3(1.f, 1.f, 1.f), skyLightColor * SKY_LIGHT_MUL);
	
	output.color.rgb = max(output.color.rgb, skyLightColor.rgb * decomp_light.a);
    output.distance = length(viewmodelpos);
    output.position = mul(viewmodelpos, u_proj);
	return output;
}

float4 PShader(PSInput input) : SV_TARGET {
    float3 fogColor = my_textureCube.SampleLevel(my_samplerLinear, input.dir, 0).rgb;
	float4 tex_color = my_texture.Sample(my_sampler, input.texCoord);
	float depth = (input.distance / 256.f);
	float alpha = input.color.a * tex_color.a;
	if (alpha < 0.1f)
		discard;
    return float4(lerp(input.color * tex_color, float4(fogColor, 1.0), min(1.0, pow(abs(depth * u_fogFactor), u_fogCurve))).rgb, alpha);
}