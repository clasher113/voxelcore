#ifdef USE_DIRECTX
#include "DXShader.hpp"
#include "../window/DXDevice.hpp"
#include "../util/DXError.hpp"
#include "../util/DebugUtil.hpp"
#include "../util/InputLayoutBuilder.hpp"
#include "../util/ConstantBufferBuilder.hpp"
#include "../../util/stringutil.hpp"

#include "../ShaderInclude.hpp"

#include <d3dcompiler.h>

Shader::Shader(ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader, ID3D11InputLayout* inputLayout, const ConstantBufferData& cbuffData) :
	ConstantBuffer(cbuffData),
	m_p_vertexShader(vertexShader),
	m_p_pixelShader(pixelShader),
	m_p_inputLayout(inputLayout)
{
	SET_DEBUG_OBJECT_NAME(m_p_vertexShader, "Vertex Shader");
	SET_DEBUG_OBJECT_NAME(m_p_pixelShader, "Pixel Shader");
	SET_DEBUG_OBJECT_NAME(m_p_inputLayout, "Input Layout");
}

Shader::~Shader() {
	m_p_vertexShader->Release();
	m_p_pixelShader->Release();
	m_p_inputLayout->Release();
}

void Shader::use() {
	ConstantBuffer::bind();
	auto context = DXDevice::getContext();
	context->VSSetShader(m_p_vertexShader, 0, 0);
	context->PSSetShader(m_p_pixelShader, 0, 0);
	context->IASetInputLayout(m_p_inputLayout);
}

void Shader::compileShader(const std::wstring_view& shaderFile, ID3D10Blob** shader, ShaderType shaderType, ID3DInclude* include) {
	ID3D10Blob* errorMsg = nullptr;
	HRESULT errorCode = S_OK;
	UINT flag1 = 0, flag2 = 0;
#ifdef _DEBUG
	flag1 = D3DCOMPILE_DEBUG;
	flag2 = D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG
	LPCSTR entryPoint = NULL, target = NULL;
	std::wstring name;
	switch (shaderType) {
	case VERTEX: entryPoint = "VShader";
		target = "vs_4_0";
		name = L"vertex";
		break;
	case PIXEL: entryPoint = "PShader";
		target = "ps_4_0";
		name = L"pixel";
		break;
	case GEOMETRY: entryPoint = "GShader";
		target = "gs_4_0";
		name = L"geometry";
		break;
	default: break;
	}

	errorCode = D3DCompileFromFile(shaderFile.data(), 0, include, entryPoint, target, flag1, flag2, shader, &errorMsg);

	//if (shader == nullptr) errorCode = S_FALSE;
	CHECK_ERROR2(errorCode, L"Failed to compile " + name + L" shader:\n" + util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));

	if (errorMsg != nullptr) {
		DXError::throwWarn(util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));
		errorMsg->Release();
	}
}

std::unique_ptr<Shader> Shader::loadShader(const std::wstring_view& shaderFile, ID3DInclude* include) {
	ID3D10Blob* VS, * PS;

	compileShader(shaderFile, &VS, ShaderType::VERTEX, include);
	compileShader(shaderFile, &PS, ShaderType::PIXEL, include);

	auto device = DXDevice::getDevice();

	ID3D11VertexShader* pVS;
	ID3D11PixelShader* pPS;

	CHECK_ERROR2(device->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS),
		L"Failed to create vertex shader:\n" + std::wstring(shaderFile));
	CHECK_ERROR2(device->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS),
		L"Failed to create pixel shader:\n" + std::wstring(shaderFile));

	ConstantBufferBuilder cbuffBuilder;
	cbuffBuilder.build(VS, ShaderType::VERTEX);
	cbuffBuilder.build(PS, ShaderType::PIXEL);

	ID3D11InputLayout* pLayout;
	CHECK_ERROR2(InputLayoutBuilder::create(device, VS, &pLayout),
		L"Failed to create input layout for shader:\n" + std::wstring(shaderFile));

	VS->Release();
	PS->Release();

	return std::make_unique<Shader>(pVS, pPS, pLayout, cbuffBuilder.getData());
}

#endif // USE_DIRECTX