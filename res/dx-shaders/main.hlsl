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
	float4x4 c_model;
	float4x4 c_projection;
	float4x4 c_view;
	float3 c_skyLightColor;
	float c_gamma;
	float3 c_cameraPos;
	float c_torchLightDistance;
	float3 c_torchLightColor;
	float c_fogFactor;
	float3 c_fogColor;
	float c_fogCurve;
	//samplerCUBE c_cubemap;
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

Texture2D my_texture : register(t0);;
SamplerState my_sampler : register(s0);;

#define SKY_LIGHT_MUL 2.5

PSInput VShader(VSInput input) {
	PSInput output;
    float4 modelpos = mul(float4(input.position, 1.f), c_model);
	float2 pos2d = modelpos.xz - c_cameraPos.xz;
    float4 viewmodelpos = mul(modelpos, c_view);
	float4 decomp_light = decompress_light(input.light);
	float3 light = decomp_light.rgb;
	float torchlight = max(0.f, 1.f - distance(c_cameraPos, modelpos.xyz) / c_torchLightDistance);
	output.dir = modelpos.xyz - c_cameraPos;
	light += torchlight * c_torchLightColor;
    output.color = float4(pow(light, float3(c_gamma, c_gamma, c_gamma)), 1.f);
	output.texCoord = input.texCoord;
	
	float3 skyLightColor = 1.f; //my_texture.Sample(my_sampler, float3(-0.4f, 0.05f, -0.4f)).rgb;
	skyLightColor.g *= 0.9;
	skyLightColor.b *= 0.8;
    skyLightColor = min(float3(1.f, 1.f, 1.f), skyLightColor * SKY_LIGHT_MUL);
	
	output.color.rgb = max(output.color.rgb, skyLightColor.rgb * decomp_light.a);
	output.distance = length(viewmodelpos);
    output.position = mul(viewmodelpos, c_projection);
	return output;
}

float4 PShader(PSInput input) : SV_TARGET {
	float3 fogColor = 1.f; //my_texture.Sample(my_sampler, input.dir);
	float4 tex_color = my_texture.Sample(my_sampler, input.texCoord);
	float depth = (input.distance / 256.f);
	float alpha = input.color.a * tex_color.a;
	if (alpha < 0.1f)
		discard;
    return input.color * tex_color; //float4(lerp(input.color * tex_color, float4(fogColor, 1.f), min(1.f, pow(depth * c_fogFactor, c_fogCurve))).rgb, alpha);
}