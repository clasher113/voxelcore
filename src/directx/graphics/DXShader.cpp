#ifdef USE_DIRECTX
#include "DXShader.hpp"
#include "../window/DXDevice.hpp"
#include "../util/DXError.hpp"
#include "../util/InputLayoutBuilder.hpp"
#include "../../util/stringutil.h"

#include <d3dcompiler.h>
#include <glm\glm.hpp>
#include <glm\gtc\type_ptr.hpp>

Shader::Shader(ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader, ID3D11InputLayout* inputLayout) :
	m_p_vertexShader(vertexShader),
	m_p_pixelShader(pixelShader),
	m_p_inputLayout(inputLayout)
{
}

Shader::~Shader() {
	m_p_vertexShader->Release();
	m_p_pixelShader->Release();
}

void Shader::use() const {
	auto context = DXDevice::getContext();
	context->VSSetShader(m_p_vertexShader, 0, 0);
	context->PSSetShader(m_p_pixelShader, 0, 0);
	context->IASetInputLayout(m_p_inputLayout);
}

Shader* Shader::loadShader(const std::wstring_view& shaderFile) {
	ID3D10Blob* VS, * PS;
	ID3D10Blob* errorMsg = nullptr;
	HRESULT errorCode = S_OK;
	UINT flag1 = 0, flag2 = 0;
#ifdef _DEBUG
	flag1 = D3DCOMPILE_DEBUG;
	flag2 = D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG
	errorCode = D3DCompileFromFile(shaderFile.data(), 0, 0, "VShader", "vs_4_0", flag1, flag2, &VS, &errorMsg);

	CHECK_ERROR2(errorCode, L"Failed to compile vertex shader:\n" +
		util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));
	if (errorMsg != nullptr) {
		DXError::throwWarn(util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));
		errorMsg->Release();
		errorMsg = nullptr;
	}
	errorCode = D3DCompileFromFile(shaderFile.data(), 0, 0, "PShader", "ps_4_0", flag1, flag2, &PS, &errorMsg);
	
	CHECK_ERROR2(errorCode, L"Failed to compile pixel shader:\n" +
		util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));

	if (errorMsg != nullptr) {
		DXError::throwWarn(util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));
		errorMsg->Release();
	}

	auto device = DXDevice::getDevice();

	ID3D11VertexShader* pVS;
	ID3D11PixelShader* pPS;

	CHECK_ERROR2(device->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS),
		L"Failed to create vertex shader:\n" + std::wstring(shaderFile));
	CHECK_ERROR2(device->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS),
		L"Failed to create pixel shader:\n" + std::wstring(shaderFile));

	InputLayoutBuilder::build(VS);

	ID3D11ShaderReflection* pReflector = NULL;
	D3DReflect(VS->GetBufferPointer(), VS->GetBufferSize(),
		IID_ID3D11ShaderReflection, (void**)&pReflector);
	D3D11_SHADER_DESC shaderDesc;
	CHECK_ERROR1(pReflector->GetDesc(&shaderDesc));

	CBuff buffer;

	for (UINT i = 0; i < shaderDesc.ConstantBuffers; i++) {
		ID3D11ShaderReflectionConstantBuffer* cBuff = pReflector->GetConstantBufferByIndex(i);
		D3D11_SHADER_BUFFER_DESC cBuffDesc;
		cBuff->GetDesc(&cBuffDesc);
		buffer.data = new unsigned char[cBuffDesc.Size];
		for (UINT j = 0; j < cBuffDesc.Variables; j++) {
			ID3D11ShaderReflectionVariable* var = cBuff->GetVariableByIndex(j);
			D3D11_SHADER_VARIABLE_DESC varDesc{};
			var->GetDesc(&varDesc);

			CBuffVar cBuffVar;
			cBuffVar.size = varDesc.Size;
			cBuffVar.startOffset = varDesc.StartOffset;

			buffer.vars.emplace(varDesc.Name, cBuffVar);
		}
	}

	ID3D11InputLayout* pLayout;
	CHECK_ERROR2(device->CreateInputLayout(InputLayoutBuilder::getData(), InputLayoutBuilder::getSize(), VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout),
		L"Failed to create input layoyt for shader:\n" + std::wstring(shaderFile));

	InputLayoutBuilder::clear();

	VS->Release();
	PS->Release();

	return new Shader(pVS, pPS, pLayout);
}

#endif // USE_DIRECTX