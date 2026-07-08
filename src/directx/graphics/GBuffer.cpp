#ifdef USE_DIRECTX
#include "GBuffer.hpp"

#include "debug/Logger.hpp"
#include "directx/util/DebugUtil.hpp"
#include "directx/window/Device.hpp"
#include "directx/util/Error.hpp"

using namespace advanced_pipeline;

static debug::Logger logger("dx-gbuffer");

void GBuffer::createColorBuffer() {
	ID3D11Device* const device = Device::getDevice();

	DXGI_SAMPLE_DESC sampleDesc{
		/* UINT Count */	1U,
		/* UINT Quality */	0U
	};

	D3D11_TEXTURE2D_DESC description{
		/* UINT Width */					m_width,
		/* UINT Height */					m_height,
		/* UINT MipLevels */				1U,
		/* UINT ArraySize */				1U,
		/* DXGI_FORMAT Format */			DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
		/* DXGI_SAMPLE_DESC SampleDesc */	sampleDesc,
		/* D3D11_USAGE Usage */				D3D11_USAGE_DEFAULT,
		/* UINT BindFlags */				D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
		/* UINT CPUAccessFlags */			0U,
		/* UINT MiscFlags */				0U
	};

	CHECK_ERROR2(device->CreateTexture2D(&description, nullptr, &m_p_colorBuffer),
		L"Failed to create texture");
	CHECK_ERROR1(device->CreateRenderTargetView(m_p_colorBuffer, nullptr, &m_p_colorBufferView));
	CHECK_ERROR2(device->CreateShaderResourceView(m_p_colorBuffer, nullptr, &m_p_colorResourceView),
		L"Failed to create shader resource view");
	SET_DEBUG_OBJECT_NAME(m_p_colorBuffer, "GBuffer color buffer");
	SET_DEBUG_OBJECT_NAME(m_p_colorBufferView, "GBuffer color RTV");
	SET_DEBUG_OBJECT_NAME(m_p_colorResourceView, "GBuffer color SRV");
}

void GBuffer::createPositionsBuffer() {
	ID3D11Device* const device = Device::getDevice();

	DXGI_SAMPLE_DESC sampleDesc{
		/* UINT Count */	1U,
		/* UINT Quality */	0U
	};

	D3D11_TEXTURE2D_DESC description{
		/* UINT Width */					m_width,
		/* UINT Height */					m_height,
		/* UINT MipLevels */				1U,
		/* UINT ArraySize */				1U,
		/* DXGI_FORMAT Format */			DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT,
		/* DXGI_SAMPLE_DESC SampleDesc */	sampleDesc,
		/* D3D11_USAGE Usage */				D3D11_USAGE_DEFAULT,
		/* UINT BindFlags */				D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
		/* UINT CPUAccessFlags */			0U,
		/* UINT MiscFlags */				0U
	};

	CHECK_ERROR2(device->CreateTexture2D(&description, nullptr, &m_p_positionsBuffer),
		L"Failed to create texture");
	CHECK_ERROR1(device->CreateRenderTargetView(m_p_positionsBuffer, nullptr, &m_p_positionsBufferView));
	CHECK_ERROR2(device->CreateShaderResourceView(m_p_positionsBuffer, nullptr, &m_p_positionsResourceView),
		L"Failed to create shader resource view");
	SET_DEBUG_OBJECT_NAME(m_p_positionsBuffer, "GBuffer positions buffer");
	SET_DEBUG_OBJECT_NAME(m_p_positionsBufferView, "GBuffer positions RTV");
	SET_DEBUG_OBJECT_NAME(m_p_positionsResourceView, "GBuffer positions SRV");
}

void GBuffer::createNormalsBuffer() {
	ID3D11Device* const device = Device::getDevice();

	DXGI_SAMPLE_DESC sampleDesc{
		/* UINT Count */	1U,
		/* UINT Quality */	0U
	};

	D3D11_TEXTURE2D_DESC description{
		/* UINT Width */					m_width,
		/* UINT Height */					m_height,
		/* UINT MipLevels */				1U,
		/* UINT ArraySize */				1U,
		/* DXGI_FORMAT Format */			DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT,
		/* DXGI_SAMPLE_DESC SampleDesc */	sampleDesc,
		/* D3D11_USAGE Usage */				D3D11_USAGE_DEFAULT,
		/* UINT BindFlags */				D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
		/* UINT CPUAccessFlags */			0U,
		/* UINT MiscFlags */				0U
	};

	CHECK_ERROR2(device->CreateTexture2D(&description, nullptr, &m_p_normalsBuffer),
		L"Failed to create texture");
	CHECK_ERROR1(device->CreateRenderTargetView(m_p_normalsBuffer, nullptr, &m_p_normalsBufferView));
	CHECK_ERROR2(device->CreateShaderResourceView(m_p_normalsBuffer, nullptr, &m_p_normalsResourceView),
		L"Failed to create shader resource view");
	SET_DEBUG_OBJECT_NAME(m_p_normalsBuffer, "GBuffer normals buffer");
	SET_DEBUG_OBJECT_NAME(m_p_normalsBufferView, "GBuffer normals RTV");
	SET_DEBUG_OBJECT_NAME(m_p_normalsResourceView, "GBuffer normals SRV");
}

void GBuffer::createEmissionBuffer() {
	ID3D11Device* const device = Device::getDevice();

	DXGI_SAMPLE_DESC sampleDesc{
		/* UINT Count */	1U,
		/* UINT Quality */	0U
	};

	D3D11_TEXTURE2D_DESC description{
		/* UINT Width */					m_width,
		/* UINT Height */					m_height,
		/* UINT MipLevels */				1U,
		/* UINT ArraySize */				1U,
		/* DXGI_FORMAT Format */			DXGI_FORMAT::DXGI_FORMAT_R8_UNORM,
		/* DXGI_SAMPLE_DESC SampleDesc */	sampleDesc,
		/* D3D11_USAGE Usage */				D3D11_USAGE_DEFAULT,
		/* UINT BindFlags */				D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
		/* UINT CPUAccessFlags */			0U,
		/* UINT MiscFlags */				0U
	};

	CHECK_ERROR2(device->CreateTexture2D(&description, nullptr, &m_p_emissionBuffer),
		L"Failed to create texture");
	CHECK_ERROR1(device->CreateRenderTargetView(m_p_emissionBuffer, nullptr, &m_p_emissionBufferView));
	CHECK_ERROR2(device->CreateShaderResourceView(m_p_emissionBuffer, nullptr, &m_p_emissionResourceView),
		L"Failed to create shader resource view");
	SET_DEBUG_OBJECT_NAME(m_p_emissionBuffer, "GBuffer emission buffer");
	SET_DEBUG_OBJECT_NAME(m_p_emissionBufferView, "GBuffer emission RTV");
	SET_DEBUG_OBJECT_NAME(m_p_emissionResourceView, "GBuffer emission SRV");
}

void GBuffer::createDepthBuffer() {
	ID3D11Device* const device = Device::getDevice();

	DXGI_SAMPLE_DESC sampleDesc{
		/* UINT Count */	1U,
		/* UINT Quality */	0U
	};

	D3D11_TEXTURE2D_DESC description{
		/* UINT Width */					m_width,
		/* UINT Height */					m_height,
		/* UINT MipLevels */				1U,
		/* UINT ArraySize */				1U,
		/* DXGI_FORMAT Format */			DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT,
		/* DXGI_SAMPLE_DESC SampleDesc */	sampleDesc,
		/* D3D11_USAGE Usage */				D3D11_USAGE_DEFAULT,
		/* UINT BindFlags */				D3D11_BIND_DEPTH_STENCIL,
		/* UINT CPUAccessFlags */			0U,
		/* UINT MiscFlags */				0U
	};

	CHECK_ERROR2(device->CreateTexture2D(&description, nullptr, &m_p_depthBuffer),
		L"Failed to create texture");
	CHECK_ERROR1(device->CreateDepthStencilView(m_p_depthBuffer, nullptr, &m_p_depthStencil));
	SET_DEBUG_OBJECT_NAME(m_p_depthBuffer, "GBuffer depth buffer");
	SET_DEBUG_OBJECT_NAME(m_p_depthStencil, "GBuffer DSV");
}

void GBuffer::createSSAOBuffer() {
	ID3D11Device* const device = Device::getDevice();

	DXGI_SAMPLE_DESC sampleDesc{
		/* UINT Count */	1U,
		/* UINT Quality */	0U
	};

	D3D11_TEXTURE2D_DESC description{
		/* UINT Width */					m_width,
		/* UINT Height */					m_height,
		/* UINT MipLevels */				1U,
		/* UINT ArraySize */				1U,
		/* DXGI_FORMAT Format */			DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT,
		/* DXGI_SAMPLE_DESC SampleDesc */	sampleDesc,
		/* D3D11_USAGE Usage */				D3D11_USAGE_DEFAULT,
		/* UINT BindFlags */				D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
		/* UINT CPUAccessFlags */			0U,
		/* UINT MiscFlags */				0U
	};

	CHECK_ERROR2(device->CreateTexture2D(&description, nullptr, &m_p_ssaoBuffer),
		L"Failed to create texture");
	CHECK_ERROR1(device->CreateRenderTargetView(m_p_ssaoBuffer, nullptr, &m_p_ssaoBufferView));
	CHECK_ERROR2(device->CreateShaderResourceView(m_p_ssaoBuffer, nullptr, &m_p_ssaoResourceView),
		L"Failed to create shader resource view");
	SET_DEBUG_OBJECT_NAME(m_p_ssaoBuffer, "GBuffer SSAO buffer");
	SET_DEBUG_OBJECT_NAME(m_p_ssaoBufferView, "GBuffer SSAO RTV");
	SET_DEBUG_OBJECT_NAME(m_p_ssaoResourceView, "GBuffer SSAO SRV");
}

GBuffer::GBuffer(uint width, uint height) : m_width(width), m_height(height) {
	createColorBuffer();
	createPositionsBuffer();
	createNormalsBuffer();
	createEmissionBuffer();
	createDepthBuffer();
	createSSAOBuffer();
}

GBuffer::~GBuffer() {
	releaseResources();
}

void GBuffer::bind() {
	ID3D11DeviceContext* const context = Device::getContext();

	ID3D11RenderTargetView* renderTargetViews[] = {
		m_p_colorBufferView,
		m_p_positionsBufferView,
		m_p_normalsBufferView,
		m_p_emissionBufferView
	};

	unbindBuffers();
	Device::setRenderTargets(renderTargetViews, m_p_depthStencil, std::size(renderTargetViews));
}

void GBuffer::unbind() {
	Device::resetRenderTarget();
}

void GBuffer::bindSSAO() const {
	Device::setRenderTargets(&m_p_ssaoBufferView);
}

void GBuffer::bindBuffers() const {
	ID3D11DeviceContext* const context = Device::getContext();

	context->PSSetShaderResources(TARGET_COLOR, 1, &m_p_colorResourceView);
	context->PSSetShaderResources(TARGET_POSITIONS, 1, &m_p_positionsResourceView);
	context->PSSetShaderResources(TARGET_NORMALS, 1, &m_p_normalsResourceView);
	context->PSSetShaderResources(TARGET_EMISSION, 1, &m_p_emissionResourceView);
}

void GBuffer::unbindBuffers() const {
	ID3D11DeviceContext* const context = Device::getContext();
	ID3D11ShaderResourceView* nullSRV = { nullptr };

	context->PSSetShaderResources(TARGET_COLOR, 1, &nullSRV);
	context->PSSetShaderResources(TARGET_POSITIONS, 1, &nullSRV);
	context->PSSetShaderResources(TARGET_NORMALS, 1, &nullSRV);
	context->PSSetShaderResources(TARGET_EMISSION, 1, &nullSRV);
}

void GBuffer::bindSSAOBuffer() const {
	ID3D11DeviceContext* const context = Device::getContext();

	context->PSSetShaderResources(TARGET_SSAO, 1, &m_p_ssaoResourceView);
}

void GBuffer::resize(uint width, uint height) {
	m_width = width;
	m_height = height;

	releaseResources();

	createColorBuffer();
	createPositionsBuffer();
	createNormalsBuffer();
	createEmissionBuffer();
	createDepthBuffer();
	createSSAOBuffer();
}

void GBuffer::bindDepthBuffer(ID3D11Texture2D* drawFbo) {
	ID3D11DeviceContext* const context = Device::getContext();

	context->CopyResource(drawFbo, m_p_depthBuffer);
}

static void releaseTexture(ID3D11Texture2D** ppTexture) {
	if (*ppTexture) {
		(*ppTexture)->Release();
		(*ppTexture) = nullptr;
	}
}

static void releaseTargetView(ID3D11RenderTargetView** ppTargetView) {
	if (*ppTargetView) {
		(*ppTargetView)->Release();
		(*ppTargetView) = nullptr;
	}
}

static void releaseShaderResourceView(ID3D11ShaderResourceView** ppShaderResourceView) {
	if (*ppShaderResourceView) {
		(*ppShaderResourceView)->Release();
		(*ppShaderResourceView) = nullptr;
	}
}

void GBuffer::releaseResources() {
	releaseTexture(&m_p_colorBuffer);
	releaseTargetView(&m_p_colorBufferView);
	releaseShaderResourceView(&m_p_colorResourceView);
	releaseTexture(&m_p_positionsBuffer);
	releaseTargetView(&m_p_positionsBufferView);
	releaseShaderResourceView(&m_p_positionsResourceView);
	releaseTexture(&m_p_normalsBuffer);
	releaseTargetView(&m_p_normalsBufferView);
	releaseShaderResourceView(&m_p_normalsResourceView);
	releaseTexture(&m_p_emissionBuffer);
	releaseTargetView(&m_p_emissionBufferView);
	releaseShaderResourceView(&m_p_emissionResourceView);
	releaseTexture(&m_p_depthBuffer);
	if (m_p_depthStencil) {
		m_p_depthStencil->Release();
		m_p_depthStencil = nullptr;
	}
	releaseTexture(&m_p_ssaoBuffer);
	releaseTargetView(&m_p_ssaoBufferView);
	releaseShaderResourceView(&m_p_ssaoResourceView);
}

#endif // USE_DIRECTX