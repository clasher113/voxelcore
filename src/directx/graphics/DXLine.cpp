#ifdef USE_DIRECTX
#include "DXLine.hpp"
#include "../window/DXDevice.hpp"
#include "../util/DXError.hpp"
#include "../ConstantBuffer.hpp"
#include "../util/ConstantBufferBuilder.hpp"
#include "DXMesh.hpp"
#include "DXShader.hpp"

#include <d3dcommon.h>

void DXLine::initialize() {
	auto device = DXDevice::getDevice();

	ID3D10Blob* GS;

	Shader::compileShader(L"res\\dx-shaders\\lines.hlsl", &GS, ShaderType::GEOMETRY);

	CHECK_ERROR1(device->CreateGeometryShader(GS->GetBufferPointer(), GS->GetBufferSize(), NULL, &s_m_p_geometryShader));

	ConstantBufferBuilder cbuffBuilder;
	cbuffBuilder.build(GS, ShaderType::GEOMETRY);

	GS->Release();

	s_m_p_cbLineWidth = new ConstantBuffer(cbuffBuilder.getData());
}

void DXLine::terminate() {
	s_m_p_geometryShader->Release();
	delete s_m_p_cbLineWidth;
}

void DXLine::setWidth(float width) {
	if (m_width == width) return;
	m_width = width;

	s_m_p_cbLineWidth->uniform1f("c_lineWidth", width);
}

void DXLine::draw(Mesh* mesh) {
	auto context = DXDevice::getContext();
	s_m_p_cbLineWidth->bind(1u);
	context->GSSetShader(s_m_p_geometryShader, 0, 0);
	mesh->draw(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	context->GSSetShader(nullptr, 0, 0);
}

#endif // USE_DIRECTX