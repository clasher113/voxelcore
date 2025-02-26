#ifdef USE_DIRECTX
#include "DXLine.hpp"
#include "directx/window/DXDevice.hpp"
#include "directx/util/DXError.hpp"
#include "directx/ConstantBuffer.hpp"
#include "directx/util/ConstantBufferBuilder.hpp"
#include "DXMesh.hpp"
#include "DXShader.hpp"

#include <d3dcompiler.h>

void DXLine::initialize(ID3D11Device* device) {
	if (s_m_initialized) {
		PRINT_ERROR2(ERROR_ALREADY_INITIALIZED, L"DXLine already initialized");
		return;
	}
	ID3D10Blob* GS;

	Shader::compileShader(L"res\\dx-shaders\\lines.hlsl", &GS, ShaderType::GEOMETRY, D3D_COMPILE_STANDARD_FILE_INCLUDE);

	CHECK_ERROR1(device->CreateGeometryShader(GS->GetBufferPointer(), GS->GetBufferSize(), NULL, &s_m_p_geometryShader));

	ConstantBufferBuilder cbuffBuilder;
	cbuffBuilder.build(GS, ShaderType::GEOMETRY);

	GS->Release();

	s_m_p_cbLineWidth = new ConstantBuffer(cbuffBuilder.getData());
	s_m_initialized = true;
}

void DXLine::terminate() {
	if (!s_m_initialized) {
		PRINT_ERROR2(E_UNEXPECTED, L"DXLine not initialized");
		return;
	}
	s_m_p_geometryShader->Release();
	delete s_m_p_cbLineWidth;
	s_m_initialized = false;
}

void DXLine::setWidth(float width) {
	if (s_m_width == width) return;
	s_m_width = width;

	s_m_p_cbLineWidth->uniform1f("c_lineWidth", width);
}

void DXLine::draw(Mesh* mesh) {
	ID3D11DeviceContext* const context = DXDevice::getContext();
	s_m_p_cbLineWidth->bind(1u);
	context->GSSetShader(s_m_p_geometryShader, 0, 0);
	mesh->draw(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	context->GSSetShader(nullptr, 0, 0);
}

#endif // USE_DIRECTX