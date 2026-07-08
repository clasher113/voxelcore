#pragma once

#include "directx/ConstantBuffer.hpp"
#include "graphics/core/PostEffect.hpp"

struct ID3D10Blob;

class ConstantBufferBuilder {
public:
	using ParamsMap = std::unordered_map<std::string, PostEffect::Param>;

	void build(ID3D10Blob* shader, ShaderType shaderType);
	static ParamsMap parseParams(ID3D10Blob* shader);

	ConstantBufferData getData();
private:
	ConstantBufferData m_data;
};