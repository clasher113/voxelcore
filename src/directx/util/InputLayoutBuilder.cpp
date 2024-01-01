#ifdef USE_DIRECTX
#include "InputLayoutBuilder.hpp"
#include "DXError.hpp"

#include <d3dcompiler.h>

std::vector<D3D11_INPUT_ELEMENT_DESC> InputLayoutBuilder::inputLayout;

void InputLayoutBuilder::build(ID3D10Blob* shader) {

	ID3D11ShaderReflection* pReflector;
	CHECK_ERROR1(D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(),
		IID_ID3D11ShaderReflection, (void**)&pReflector));
	D3D11_SHADER_DESC shaderDesc;
	CHECK_ERROR1(pReflector->GetDesc(&shaderDesc));

	D3D11_SIGNATURE_PARAMETER_DESC paramDesc;

	for (UINT i = 0; i < shaderDesc.InputParameters; i++) {
		CHECK_ERROR1(pReflector->GetInputParameterDesc(i, &paramDesc));

		D3D11_INPUT_ELEMENT_DESC elemDesc{};
		elemDesc.SemanticName = paramDesc.SemanticName;
		elemDesc.SemanticIndex = paramDesc.SemanticIndex;
		if (paramDesc.Mask == 1) {
			switch (paramDesc.ComponentType) {
				case D3D_REGISTER_COMPONENT_UINT32: elemDesc.Format = DXGI_FORMAT_R32_UINT; break;
				case D3D_REGISTER_COMPONENT_SINT32: elemDesc.Format = DXGI_FORMAT_R32_SINT; break;
				case D3D_REGISTER_COMPONENT_FLOAT32: elemDesc.Format = DXGI_FORMAT_R32_FLOAT; break;
			}
		}
		else if (paramDesc.Mask <= 3) {
			switch (paramDesc.ComponentType) {
				case D3D_REGISTER_COMPONENT_UINT32: elemDesc.Format = DXGI_FORMAT_R32G32_UINT; break;
				case D3D_REGISTER_COMPONENT_SINT32: elemDesc.Format = DXGI_FORMAT_R32G32_SINT; break;
				case D3D_REGISTER_COMPONENT_FLOAT32: elemDesc.Format = DXGI_FORMAT_R32G32_FLOAT; break;
			}
		}
		else if (paramDesc.Mask <= 7) {
			switch (paramDesc.ComponentType) {
				case D3D_REGISTER_COMPONENT_UINT32: elemDesc.Format = DXGI_FORMAT_R32G32B32_UINT; break;
				case D3D_REGISTER_COMPONENT_SINT32: elemDesc.Format = DXGI_FORMAT_R32G32B32_SINT; break;
				case D3D_REGISTER_COMPONENT_FLOAT32: elemDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT; break;
			}
		}
		else if (paramDesc.Mask <= 15) {
			switch (paramDesc.ComponentType) {
				case D3D_REGISTER_COMPONENT_UINT32: elemDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT; break;
				case D3D_REGISTER_COMPONENT_SINT32: elemDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT; break;
				case D3D_REGISTER_COMPONENT_FLOAT32: elemDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			}
		}
		elemDesc.InputSlot = 0u;
		elemDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		elemDesc.InstanceDataStepRate = 0u;
		inputLayout.emplace_back(elemDesc);
	}
}

void InputLayoutBuilder::clear() {
	inputLayout.clear();
}

UINT InputLayoutBuilder::getSize() {
	return static_cast<UINT>(inputLayout.size());
}

D3D11_INPUT_ELEMENT_DESC* InputLayoutBuilder::getData() {
	return inputLayout.data();
}

#endif // USE_DIRECTX