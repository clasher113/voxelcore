#ifdef USE_DIRECTX
#include "ConstantBufferBuilder.hpp"

#include "Error.hpp"

#include <d3dcompiler.h>

void ConstantBufferBuilder::build(ID3D10Blob* shader, ShaderType shaderType) {
	ID3D11ShaderReflection* pReflector = nullptr;
	CHECK_ERROR1(D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&pReflector)));

	D3D11_SHADER_DESC shaderDesc{};
	CHECK_ERROR1(pReflector->GetDesc(&shaderDesc));

	for (UINT i = 0; i < shaderDesc.ConstantBuffers; i++) {
		ID3D11ShaderReflectionConstantBuffer* cBuff = pReflector->GetConstantBufferByIndex(i);

		D3D11_SHADER_BUFFER_DESC cBuffDesc;
		cBuff->GetDesc(&cBuffDesc);

		auto found = m_data.data.find(cBuffDesc.Name);
		if (found != m_data.data.end()) {
			ConstantBufferData::Data& data = found->second;
			data.shaderType = static_cast<ShaderType>(data.shaderType | shaderType);
			continue;
		}

		D3D11_SHADER_INPUT_BIND_DESC bindDesc;
		pReflector->GetResourceBindingDescByName(cBuffDesc.Name, &bindDesc);

		ConstantBufferData::Data data;
		data.shaderType = shaderType;
		data.bindSlot = bindDesc.BindPoint;

		for (UINT j = 0; j < cBuffDesc.Variables; j++) {
			ID3D11ShaderReflectionVariable* var = cBuff->GetVariableByIndex(j);

			D3D11_SHADER_VARIABLE_DESC varDesc{};
			var->GetDesc(&varDesc);

			ConstantBufferVariable cBuffVar{
				/* size_t startOffset */	varDesc.StartOffset,
				/* size_t size */			varDesc.Size
			};

			data.bufferVars.emplace(varDesc.Name, cBuffVar);
		}
		m_data.data.emplace(cBuffDesc.Name, data);
	}
	pReflector->Release();
}

ConstantBufferBuilder::ParamsMap ConstantBufferBuilder::parseParams(ID3D10Blob* shader) {
	ParamsMap params;

	ID3D11ShaderReflection* pReflector = nullptr;
	CHECK_ERROR1(D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&pReflector)));

	D3D11_SHADER_DESC shaderDesc{};
	CHECK_ERROR1(pReflector->GetDesc(&shaderDesc));

	for (UINT i = 0; i < shaderDesc.ConstantBuffers; i++) {
		ID3D11ShaderReflectionConstantBuffer* cBuff = pReflector->GetConstantBufferByIndex(i);

		D3D11_SHADER_BUFFER_DESC cBuffDesc;
		cBuff->GetDesc(&cBuffDesc);

		if (std::string(cBuffDesc.Name) != "Params") continue;

		for (UINT j = 0; j < cBuffDesc.Variables; j++) {
			ID3D11ShaderReflectionVariable* var = cBuff->GetVariableByIndex(j);
			ID3D11ShaderReflectionType* varType = var->GetType();

			D3D11_SHADER_VARIABLE_DESC varDesc{};
			var->GetDesc(&varDesc);

			D3D11_SHADER_TYPE_DESC typeDesc{};
			varType->GetDesc(&typeDesc);

			PostEffect::Param param;
			param.array = typeDesc.Elements != 0;
			
			switch (typeDesc.Type) {
				case D3D_SHADER_VARIABLE_TYPE::D3D10_SVT_INT:
					param.type = PostEffect::Param::Type::INT;
					param.defValue = varDesc.DefaultValue ? *static_cast<int*>(varDesc.DefaultValue) : 0;
					break;
				case D3D_SHADER_VARIABLE_TYPE::D3D10_SVT_FLOAT:
					switch (typeDesc.Columns) {
						case 1:
							param.type = PostEffect::Param::Type::FLOAT;
							param.defValue = varDesc.DefaultValue ? *static_cast<float*>(varDesc.DefaultValue) : 0.f;
							break;
						case 2:
							param.type = PostEffect::Param::Type::VEC2;
							param.defValue = varDesc.DefaultValue ? *static_cast<glm::vec2*>(varDesc.DefaultValue) : glm::vec2(0.f);
							break;
						case 3:
							param.type = PostEffect::Param::Type::VEC3;
							param.defValue = varDesc.DefaultValue ? *static_cast<glm::vec3*>(varDesc.DefaultValue) : glm::vec3(0.f);
							break;
						case 4:
							param.type = PostEffect::Param::Type::VEC4;
							param.defValue = varDesc.DefaultValue ? *static_cast<glm::vec4*>(varDesc.DefaultValue) : glm::vec4(0.f);
							break;
					}
					break;
			}
			param.value = param.defValue;

			params.emplace(varDesc.Name, param);
		}

		break;
	}

	return params;
}

ConstantBufferData ConstantBufferBuilder::getData() {
	return m_data;
}

#endif // USE_DIRECTX

