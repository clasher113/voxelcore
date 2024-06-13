#ifdef USE_DIRECTX
#include "ConstantBufferBuilder.hpp"
#include "DXError.hpp"

void ConstantBufferBuilder::build(ID3D10Blob* shader, unsigned int shaderType) {
	ID3D11ShaderReflection* pReflector;
	CHECK_ERROR1(D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&pReflector)));
	D3D11_SHADER_DESC shaderDesc;
	CHECK_ERROR1(pReflector->GetDesc(&shaderDesc));
	if (shaderDesc.ConstantBuffers != 0) m_shaderType |= shaderType;
	for (UINT i = 0; i < shaderDesc.ConstantBuffers; i++) {
		ID3D11ShaderReflectionConstantBuffer* cBuff = pReflector->GetConstantBufferByIndex(i);
		D3D11_SHADER_BUFFER_DESC cBuffDesc;
		cBuff->GetDesc(&cBuffDesc);
		for (UINT j = 0; j < cBuffDesc.Variables; j++) {
			ID3D11ShaderReflectionVariable* var = cBuff->GetVariableByIndex(j);
			D3D11_SHADER_VARIABLE_DESC varDesc{};
			var->GetDesc(&varDesc);

			ConstantBufferVariable cBuffVar;
			cBuffVar.startOffset = varDesc.StartOffset;
			cBuffVar.size = varDesc.Size;
			m_size += varDesc.Size;

			m_bufferVars.emplace(varDesc.Name, cBuffVar);
		}
	}
	pReflector->Release();
}

ConstantBufferData ConstantBufferBuilder::getData() {
	ConstantBufferData result;
	result.bufferVars = std::move(m_bufferVars);
	result.shaderType = m_shaderType;
	result.size = m_size;
	return result;
}

#endif // USE_DIRECTX

