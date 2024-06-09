#ifdef USE_DIRECTX
#include "DXShader.hpp"
#include "../window/DXDevice.hpp"
#include "../util/DXError.hpp"
#include "../util/DebugUtil.hpp"
#include "../util/InputLayoutBuilder.hpp"
#include "../util/ConstantBufferBuilder.hpp"
#include "../../util/stringutil.h"

#include <d3dcompiler.h>

Shader::Shader(ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader, ID3D11InputLayout* inputLayout, ConstantBuffer* cBuffer) :
	ConstantBuffer(*cBuffer),
	m_p_vertexShader(vertexShader),
	m_p_pixelShader(pixelShader),
	m_p_inputLayout(inputLayout)
{
#ifdef _DEBUG
	SetDebugObjectName(m_p_vertexShader, "Vertex Shader");
	SetDebugObjectName(m_p_pixelShader, "Pixel Shader");
	SetDebugObjectName(m_p_inputLayout, "Input Layout");
#endif // _DEBUG
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

void Shader::compileShader(const std::wstring_view& shaderFile, ID3D10Blob** shader, ShaderType shaderType) {
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

	errorCode = D3DCompileFromFile(shaderFile.data(), 0, 0, entryPoint, target, flag1, flag2, shader, &errorMsg);

	//if (shader == nullptr) errorCode = S_FALSE;
	CHECK_ERROR2(errorCode, L"Failed to compile " + name + L" shader:\n" + util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));

	if (errorMsg != nullptr) {
		DXError::throwWarn(util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));
		errorMsg->Release();
	}
}

Shader* Shader::loadShader(const std::wstring_view& shaderFile) {
	ID3D10Blob* VS, * PS;

	compileShader(shaderFile, &VS, ShaderType::VERTEX);
	compileShader(shaderFile, &PS, ShaderType::PIXEL);

	auto device = DXDevice::getDevice();

	ID3D11VertexShader* pVS;
	ID3D11PixelShader* pPS;

	CHECK_ERROR2(device->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS),
		L"Failed to create vertex shader:\n" + std::wstring(shaderFile));
	CHECK_ERROR2(device->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS),
		L"Failed to create pixel shader:\n" + std::wstring(shaderFile));

	ConstantBufferBuilder::build(VS, ShaderType::VERTEX);
	ConstantBufferBuilder::build(PS, ShaderType::PIXEL);
	ConstantBuffer* cBuff = ConstantBufferBuilder::create();

	ID3D11InputLayout* pLayout;
	CHECK_ERROR2(InputLayoutBuilder::create(device, VS, &pLayout),
		L"Failed to create input layout for shader:\n" + std::wstring(shaderFile));

	ConstantBufferBuilder::clear();

	VS->Release();
	PS->Release();

	return new Shader(pVS, pPS, pLayout, cBuff);
}

#endif // USE_DIRECTX