#ifdef USE_DIRECTX
#include "Shader.hpp"

#include "directx/window/Device.hpp"
#include "directx/util/Error.hpp"
#include "directx/util/DebugUtil.hpp"
#include "directx/util/InputLayoutBuilder.hpp"
#include "directx/util/ConstantBufferBuilder.hpp"
#include "util/stringutil.hpp"
#include "directx/ShaderInclude.hpp"
#include "debug/Logger.hpp"

#include <d3dcompiler.h>
#include <wrl/client.h>

static debug::Logger logger("dx-shader");

Shader* Shader::used = nullptr;

Shader::Shader(const ShaderBundle& shaderBundle, const ConstantBufferData& cbuffData, std::string sourceCode) : ConstantBuffer(cbuffData),
	m_p_vertexShader(shaderBundle.vertexShader),
	m_p_pixelShader(shaderBundle.pixelShader),
	m_p_inputLayout(shaderBundle.inputLayout),
	m_sourceCode(std::move(sourceCode))
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
	ID3D11DeviceContext* const context = Device::getContext();
	context->VSSetShader(m_p_vertexShader, nullptr, 0U);
	context->PSSetShader(m_p_pixelShader, nullptr, 0U);
	context->IASetInputLayout(m_p_inputLayout);
	used = this;
}

void Shader::recompile(const std::vector<std::string>& defines) {
	std::unique_ptr<Shader> recompiled = loadShader("<runtime>", m_sourceCode, defines);

	if (recompiled) {
		static auto toHex = [](void* ptr) {
			std::stringstream sstream;
			sstream << "0x" << std::hex << reinterpret_cast<ptrdiff_t>(ptr);
			return std::string(sstream.str());
		};

		logger.info() << "shaders " << toHex(recompiled->m_p_pixelShader) << " " << toHex(recompiled->m_p_pixelShader) << " has been recompiled";

		this->~Shader();
		new (this) Shader(*recompiled.release());
	}
}

ID3D10Blob* Shader::compileShader(const std::string& shaderSource, ShaderType shaderType, const std::vector<std::string>& defines) {
	UINT flag1 = 0U, flag2 = 0U;
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
	if (entryPoint == NULL || target == NULL) {
		return nullptr;
	}

	std::vector<D3D_SHADER_MACRO> macros;

	if (!defines.empty()) {
		for (const auto& name : defines) {
			macros.emplace_back(D3D_SHADER_MACRO{ name.c_str(), "" });
		}
		macros.emplace_back(D3D_SHADER_MACRO{ NULL, NULL });
	}

	Microsoft::WRL::ComPtr<ID3D10Blob> errorMsg = nullptr;

	HRESULT errorCode = S_OK;

	ShaderInclude include;
	
	ID3D10Blob* shaderByteCode = nullptr;
	errorCode = D3DCompile(shaderSource.data(), shaderSource.size(), util::wstr2str_utf8(name).c_str(), 
		macros.empty() ? NULL : macros.data(), &include, entryPoint, target, flag1, flag2, &shaderByteCode, errorMsg.GetAddressOf());

	if (errorMsg != nullptr) {
		CHECK_ERROR2(errorCode, L"Failed to compile " + name + L" shader:\n" + util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));
		Error::throwWarn(util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));
		errorMsg->Release();
	}

	return shaderByteCode;
}

std::unique_ptr<Shader> Shader::loadShader(const std::string& fileName, const std::string& shaderSource,
	const std::vector<std::string>& defines)
{

	ID3DBlob* vertexByteCode = compileShader(shaderSource, ShaderType::VERTEX, defines);
	ID3DBlob* pixelByteCode = compileShader(shaderSource, ShaderType::PIXEL, defines);

	ID3D11Device* const device = Device::getDevice();

	ID3D11VertexShader* vertexShader = nullptr;
	ID3D11PixelShader* pixelShader = nullptr;

	CHECK_ERROR2(device->CreateVertexShader(vertexByteCode->GetBufferPointer(), vertexByteCode->GetBufferSize(), NULL, &vertexShader),
		L"Failed to create vertex shader:\n" + util::str2wstr_utf8(fileName));
	CHECK_ERROR2(device->CreatePixelShader(pixelByteCode->GetBufferPointer(), pixelByteCode->GetBufferSize(), NULL, &pixelShader),
		L"Failed to create pixel shader:\n" + util::str2wstr_utf8(fileName));

	ConstantBufferBuilder cbuffBuilder;
	cbuffBuilder.build(vertexByteCode, ShaderType::VERTEX);
	cbuffBuilder.build(pixelByteCode, ShaderType::PIXEL);

	ID3D11InputLayout* pLayout;
	CHECK_ERROR2(InputLayoutBuilder::create(device, vertexByteCode, &pLayout),
		L"Failed to create input layout for shader:\n" + util::str2wstr_utf8(fileName));

	vertexByteCode->Release();
	pixelByteCode->Release();

	return std::make_unique<Shader>(ShaderBundle{ vertexShader, pixelShader, pLayout }, cbuffBuilder.getData(), shaderSource);
}

Shader& Shader::getUsed() {
	return *used;
}

#endif // USE_DIRECTX