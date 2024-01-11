#ifdef USE_DIRECTX
#include "ConstantBufferBuilder.hpp"
#include "DXError.hpp"

size_t ConstantBufferBuilder::s_m_size = 0;
unsigned int ConstantBufferBuilder::s_m_shaderType;
std::unordered_map<std::string, ConstantBufferVariable> ConstantBufferBuilder::s_m_bufferVars;

void ConstantBufferBuilder::build(ID3D10Blob* shader, unsigned int shaderType) {
	ID3D11ShaderReflection* pReflector;
	CHECK_ERROR1(D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&pReflector)));
	D3D11_SHADER_DESC shaderDesc;
	CHECK_ERROR1(pReflector->GetDesc(&shaderDesc));
	if (shaderDesc.ConstantBuffers != 0) s_m_shaderType |= shaderType;
	for (UINT i = 0; i < shaderDesc.ConstantBuffers; i++) {
		ID3D11ShaderReflectionConstantBuffer* cBuff = pReflector->GetConstantBufferByIndex(i);
		D3D11_SHADER_BUFFER_DESC cBuffDesc;
		cBuff->GetDesc(&cBuffDesc);
		for (UINT j = 0; j < cBuffDesc.Variables; j++) {
			ID3D11ShaderReflectionVariable* var = cBuff->GetVariableByIndex(j);
			D3D11_SHADER_VARIABLE_DESC varDesc{};
			var->GetDesc(&varDesc);

			if (s_m_bufferVars.find(varDesc.Name) != s_m_bufferVars.end()) continue;

			ConstantBufferVariable cBuffVar;
			cBuffVar.startOffset = varDesc.StartOffset;
			cBuffVar.size = varDesc.Size;
			s_m_size += varDesc.Size;

			s_m_bufferVars.emplace(varDesc.Name, cBuffVar);
		}
	}
	pReflector->Release();
}

void ConstantBufferBuilder::clear() {
	s_m_bufferVars.clear();
	s_m_size = 0;
}

ConstantBuffer* ConstantBufferBuilder::create() {
	ConstantBuffer* cBuff = new ConstantBuffer(s_m_size, s_m_shaderType);
	for (const auto& elem : s_m_bufferVars) {
		cBuff->addVariable(elem.first, elem.second);
	}
	return cBuff;
}

#endif // USE_DIRECTX

