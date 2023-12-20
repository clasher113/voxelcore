#ifdef USE_DIRECTX
#include "DXShader.hpp"
#include "../window/DXDevice.hpp"
#include "../util/DXError.hpp"
#include "../util/InputLayoutBuilder.hpp"
#include "../../util/stringutil.h"

#include <D3DX11async.h>
#include <iostream>
#ifdef _DEBUG
#include <D3DCompiler.h>
#endif // _DEBUG

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
	ID3D10Blob* VS, *PS;
	ID3D10Blob* errorMsg;
	HRESULT errorCode = S_OK;
	errorCode = D3DX11CompileFromFileW(shaderFile.data(), 0, 0, "VShader", "vs_4_0", 0, 0, 0, &VS, &errorMsg, 0);
	if (FAILED(errorCode)) {
		DXError::throwError(errorCode, L"Failed to compile vertex shader:\n" +
			std::wstring(util::str2wstr_utf8((char*)errorMsg->GetBufferPointer())));
	}
	errorCode = D3DX11CompileFromFileW(shaderFile.data(), 0, 0, "PShader", "ps_4_0", 0, 0, 0, &PS, &errorMsg, 0);
	if (FAILED(errorCode)) {
		DXError::throwError(errorCode, L"Failed to compile pixel shader:\n" +
			std::wstring(util::str2wstr_utf8((char*)errorMsg->GetBufferPointer())));
	}

	auto device = DXDevice::getDevice();

	ID3D11VertexShader* pVS;
	ID3D11PixelShader* pPS;

	DXError::checkError(device->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS),
		L"Failed to create vertex shader: \m" + std::wstring(shaderFile));
	DXError::checkError(device->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS),
		L"Failed to create pixel shader: \n" + std::wstring(shaderFile));

	auto context = DXDevice::getContext();

	static int index = 0;
	switch (index) {
	case 0: InputLayoutBuilder::add("POSITION", 3);
			InputLayoutBuilder::add("TEXCOORD", 2);
			InputLayoutBuilder::add("LIGHT", 1);
			break; // main
	case 1:	InputLayoutBuilder::add("POSITION", 3);
			InputLayoutBuilder::add("COLOR", 4);
			break; // lines
	case 2: InputLayoutBuilder::add("POSITION", 2);
			InputLayoutBuilder::add("UV", 2);
			InputLayoutBuilder::add("COLOR", 4);
			break; // ui
	case 3:	InputLayoutBuilder::add("POSITION", 3);
			InputLayoutBuilder::add("TEXCOORD", 2);
			InputLayoutBuilder::add("COLOR", 4);
			break; // ui3d
	case 4:
	case 5: InputLayoutBuilder::add("POSITION", 2);
			break; // background & skybox
	}

	ID3D11InputLayout* pLayout;
	DXError::checkError(device->CreateInputLayout(InputLayoutBuilder::get(), InputLayoutBuilder::getSize(), VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout),
		L"Failed to create input layoyt for shader:\m" + std::wstring(shaderFile));

	InputLayoutBuilder::clear();
	index++;

	VS->Release();
	PS->Release();

	return new Shader(pVS, pPS, pLayout);
}

#endif // USE_DIRECTX