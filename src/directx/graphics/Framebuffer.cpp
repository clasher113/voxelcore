#ifdef USE_DIRECTX
#include "Framebuffer.hpp"

#include "directx/window/Device.hpp"
#include "directx/util/Error.hpp"
#include "directx/util/DebugUtil.hpp"
#include "Texture.hpp"
#include "Cubemap.hpp"

#include <d3d11.h>
#include <type_traits>

Framebuffer::Framebuffer(std::unique_ptr<Texture> texture) :
	m_width(texture->getWidth()),
	m_height(texture->getHeight()),
	m_p_depthTexture(nullptr),
	m_p_depthStencil(nullptr)
{
	ID3D11Device* const device = Device::getDevice();

	D3D11_TEXTURE2D_DESC description;
	texture->getId()->GetDesc(&description);

	if (description.BindFlags & D3D11_BIND_RENDER_TARGET) {
		m_p_texture = std::move(texture);
	}
	else {
		ID3D11DeviceContext* const context = Device::getContext();

		description.BindFlags |= D3D11_BIND_RENDER_TARGET;

		ID3D11Texture2D* tex = nullptr;

		CHECK_ERROR2(device->CreateTexture2D(&description, nullptr, &tex),
			L"Failed to create texture");

		if (tex) {
			context->CopyResource(tex, texture->getId());
		}

		if (dynamic_cast<Cubemap*>(texture.get())) {
			m_p_texture = std::make_unique<Cubemap>(tex);
		}
		else {
			m_p_texture = std::make_unique<Texture>(tex);
		}

		texture.reset();
	}

	m_renderTargetCount = description.ArraySize;
	m_p_renderTarget = new ID3D11RenderTargetView * [m_renderTargetCount];

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{
		/* DXGI_FORMAT Format */					description.Format,
		/* D3D11_RTV_DIMENSION ViewDimension */		m_renderTargetCount > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DARRAY : D3D11_RTV_DIMENSION_TEXTURE2D,
		/* D3D11_TEX2D_ARRAY_RTV Texture2DArray */{
		/* UINT MipSlice */			0U,
		/* UINT FirstArraySlice */	1U
		}
	};
	rtvDesc.Texture2DArray.ArraySize = 1;

	for (size_t i = 0; i < m_renderTargetCount; i++) {
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		CHECK_ERROR1(device->CreateRenderTargetView(m_p_texture->getId(), &rtvDesc, &m_p_renderTarget[i]));
		SET_DEBUG_OBJECT_NAME(m_p_renderTarget[0], std::string("Framebuffer RTV [" + std::to_string(i) + "]").c_str());
	}
}

static ID3D11Texture2D* create_texture(UINT width, UINT height, DXGI_FORMAT format, UINT bindFlags) {
	ID3D11Device* const device = Device::getDevice();

	D3D11_TEXTURE2D_DESC description {
		/* UINT Width */					width,
		/* UINT Height */					height,
		/* UINT MipLevels */				1U,
		/* UINT ArraySize */				1U,
		/* DXGI_FORMAT Format */			format,
		/* DXGI_SAMPLE_DESC SampleDesc */	{
			/* UINT Count */	1U,
			/* UINT Quality */	0U
		},
		/* D3D11_USAGE Usage */				D3D11_USAGE_DEFAULT,
		/* UINT BindFlags */				bindFlags,
		/* UINT CPUAccessFlags */			0U,
		/* UINT MiscFlags */				0U
	};

	ID3D11Texture2D* tex = nullptr;

	CHECK_ERROR2(device->CreateTexture2D(&description, nullptr, &tex),
		L"Failed to create texture");

	return tex;
}

Framebuffer::Framebuffer(uint width, uint height, bool alpha) :
	m_width(0),
	m_height(0),
	m_renderTargetCount(0)
{
	resize(width, height);
}

Framebuffer::~Framebuffer() {
	releaseResources();
}

Texture* Framebuffer::getTexture() const {
	return m_p_texture.get();
}

void Framebuffer::resize(uint width, uint height) {
	if (m_width == width && m_height == height) {
		return;
	}
	m_width = width;
	m_height = height;

	ID3D11Device* const device = Device::getDevice();

	releaseResources();

	m_renderTargetCount = 1;
	m_p_renderTarget = new ID3D11RenderTargetView*[m_renderTargetCount];

	m_p_texture = std::make_shared<Texture>(create_texture(m_width, m_height, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET));
	CHECK_ERROR1(device->CreateRenderTargetView(m_p_texture->getId(), nullptr, &m_p_renderTarget[0]));

	m_p_depthTexture = create_texture(m_width, m_height, DXGI_FORMAT_D32_FLOAT, D3D11_BIND_DEPTH_STENCIL);
	CHECK_ERROR1(device->CreateDepthStencilView(m_p_depthTexture, nullptr, &m_p_depthStencil));

	SET_DEBUG_OBJECT_NAME(m_p_texture->getId(), "Framebuffer color buffer");
	SET_DEBUG_OBJECT_NAME(m_p_renderTarget[0], "Framebuffer RTV");
	SET_DEBUG_OBJECT_NAME(m_p_depthTexture, "Framebuffer depth stencil buffer");
	SET_DEBUG_OBJECT_NAME(m_p_depthStencil, "Framebuffer DSV");
}

ID3D11Texture2D* Framebuffer::getDepthTexture() {
	return m_p_depthTexture;
}

void Framebuffer::bind(size_t index) {
	if (index > m_renderTargetCount) return;
	Device::setRenderTargets(&m_p_renderTarget[index], m_p_depthStencil);
}

void Framebuffer::bind() {
	Device::setRenderTargets(&m_p_renderTarget[0], m_p_depthStencil);
}

void Framebuffer::unbind() {
	Device::resetRenderTarget();
}

void Framebuffer::releaseResources() {
	for (size_t i = 0; i < m_renderTargetCount; i++) {
		m_p_renderTarget[i]->Release();
	}
	delete[] m_p_renderTarget;
	if (m_p_depthStencil) m_p_depthStencil->Release();
	if (m_p_depthTexture) m_p_depthTexture->Release();
}

std::shared_ptr<Texture> Framebuffer::getSharedTexture() const {
	return m_p_texture;
}

uint Framebuffer::getWidth() const {
	return m_width;
}

uint Framebuffer::getHeight() const {
	return m_height;
}

ID3D11RenderTargetView* Framebuffer::getRTV(size_t index) {
	if (index >= m_renderTargetCount) return nullptr;
	return m_p_renderTarget[index];
}

ID3D11DepthStencilView* Framebuffer::getDSV() {
	return m_p_depthStencil;
}

#endif // USE_DIRECTX