#ifdef USE_DIRECTX
#include "LineRenderer.hpp"

#include "directx/ConstantBuffer.hpp"
#include "directx/util/ConstantBufferBuilder.hpp"
#include "directx/util/Error.hpp"
#include "directx/window/Device.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"
#include "graphics/core/Batch2D.hpp"
#include "graphics/core/LineBatch.hpp"
#include "constants.hpp"
#include "io/io.hpp"

#include <d3dcompiler.h>

void LineRenderer::initialize(ID3D11Device* device) {
	if (s_m_initialized) {
		PRINT_ERROR2(ERROR_ALREADY_INITIALIZED, L"LineRenderer already initialized");
		return;
	}

	const io::path shaderFile = io::path("res:" + SHADERS_FOLDER + "/lines.hlsl");
	const std::string shaderSource = io::read_string(shaderFile);

	ID3D10Blob* geometryByteCode = Shader::compileShader(shaderSource, ShaderType::GEOMETRY);

	CHECK_ERROR1(device->CreateGeometryShader(geometryByteCode->GetBufferPointer(), geometryByteCode->GetBufferSize(), NULL,
		&s_m_p_geometryShader));

	ConstantBufferBuilder cbuffBuilder;
	cbuffBuilder.build(geometryByteCode, ShaderType::GEOMETRY);

	geometryByteCode->Release();

	s_m_p_cbLineWidth = new ConstantBuffer(cbuffBuilder.getData());
	s_m_initialized = true;
}

void LineRenderer::terminate() {
	if (!s_m_initialized) {
		PRINT_ERROR2(E_UNEXPECTED, L"LineRenderer not initialized");
		return;
	}
	s_m_p_geometryShader->Release();
	delete s_m_p_cbLineWidth;
	s_m_initialized = false;
}

void LineRenderer::setWidth(float width) {
	if (s_m_width == width) return;
	s_m_width = width;

	s_m_p_cbLineWidth->uniform1f("c_lineWidth", width);
}

template void LineRenderer::draw(const Mesh<Batch2DVertex>&);
template void LineRenderer::draw(const Mesh<LineVertex>&);

template<typename VertexStructure>
void LineRenderer::draw(const Mesh<VertexStructure>& mesh) {
	ID3D11DeviceContext* const context = Device::getContext();
	s_m_p_cbLineWidth->bind();
	context->GSSetShader(s_m_p_geometryShader, 0, 0);
	mesh.draw(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	context->GSSetShader(nullptr, 0, 0);
}

#endif // USE_DIRECTX