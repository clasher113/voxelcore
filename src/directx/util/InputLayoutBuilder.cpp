#ifdef USE_DIRECTX
#include "InputLayoutBuilder.hpp"

#include "Error.hpp"

#include <unordered_map>
#include <d3dcompiler.h>

static uint8_t countComponents(BYTE mask) {
	for (uint8_t i = 0; i < 8; i++) {
		if (!(mask & 1ull << i)) {
			return i;
		}
	}
	return 0;
}

static DXGI_FORMAT getFormat(const std::string_view& semanticName, UINT mask, D3D_REGISTER_COMPONENT_TYPE componentType) {
	const static std::unordered_map<const char*, DXGI_FORMAT> customTypes {
		{ "uint8", DXGI_FORMAT_R8G8B8A8_UINT },
		{ "int8_unorm", DXGI_FORMAT_R8G8B8A8_UNORM },
		{ "int8_snorm", DXGI_FORMAT_R8G8B8A8_SNORM }
	};

	for (const auto& [name, format] : customTypes) {
		if (semanticName.find(name) != std::string::npos) {
			return format;
		}
	}

	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	uint8_t components = countComponents(mask);

	switch (components) {
		case 1:
			switch (componentType) {
				case D3D_REGISTER_COMPONENT_UINT32: format = DXGI_FORMAT_R32_UINT; break;
				case D3D_REGISTER_COMPONENT_SINT32: format = DXGI_FORMAT_R32_SINT; break;
				case D3D_REGISTER_COMPONENT_FLOAT32: format = DXGI_FORMAT_R32_FLOAT; break;
			}
			break;
		case 2:
			switch (componentType) {
				case D3D_REGISTER_COMPONENT_UINT32: format = DXGI_FORMAT_R32G32_UINT; break;
				case D3D_REGISTER_COMPONENT_SINT32: format = DXGI_FORMAT_R32G32_SINT; break;
				case D3D_REGISTER_COMPONENT_FLOAT32: format = DXGI_FORMAT_R32G32_FLOAT; break;
			}
			break;
		case 3:
			switch (componentType) {
				case D3D_REGISTER_COMPONENT_UINT32: format = DXGI_FORMAT_R32G32B32_UINT; break;
				case D3D_REGISTER_COMPONENT_SINT32: format = DXGI_FORMAT_R32G32B32_SINT; break;
				case D3D_REGISTER_COMPONENT_FLOAT32: format = DXGI_FORMAT_R32G32B32_FLOAT; break;
			}
			break;
		case 4:
			switch (componentType) {
				case D3D_REGISTER_COMPONENT_UINT32: format = DXGI_FORMAT_R32G32B32A32_UINT; break;
				case D3D_REGISTER_COMPONENT_SINT32: format = DXGI_FORMAT_R32G32B32A32_SINT; break;
				case D3D_REGISTER_COMPONENT_FLOAT32: format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			}
			break;
		default: break;
	}

	return format;
}

bool InputLayoutBuilder::create(ID3D11Device* device, ID3D10Blob* shader, ID3D11InputLayout** inputLayout) {
	ID3D11ShaderReflection* pReflector;
	CHECK_ERROR1(D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&pReflector)));

	D3D11_SHADER_DESC shaderDesc;
	CHECK_ERROR1(pReflector->GetDesc(&shaderDesc));

	D3D11_INPUT_ELEMENT_DESC* elements = new D3D11_INPUT_ELEMENT_DESC[shaderDesc.InputParameters];
	ZeroMemory(elements, sizeof(D3D11_INPUT_ELEMENT_DESC) * shaderDesc.InputParameters);

	for (UINT i = 0; i < shaderDesc.InputParameters; i++) {
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
		CHECK_ERROR1(pReflector->GetInputParameterDesc(i, &paramDesc));

		D3D11_INPUT_ELEMENT_DESC& elemDesc = elements[i];
		elemDesc.SemanticName = paramDesc.SemanticName;
		elemDesc.SemanticIndex = paramDesc.SemanticIndex;
		elemDesc.Format = getFormat(paramDesc.SemanticName, paramDesc.Mask, paramDesc.ComponentType);
		elemDesc.InputSlot = 0u;
		elemDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		elemDesc.InstanceDataStepRate = 0u;
	}
	HRESULT hr = device->CreateInputLayout(elements, shaderDesc.InputParameters,
		shader->GetBufferPointer(), shader->GetBufferSize(), inputLayout);

	delete[] elements;

	pReflector->Release();
	return hr == S_OK;
}

#endif // USE_DIRECTX