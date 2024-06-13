#ifdef USE_DIRECTX
#include "DXFramebuffer.hpp"
#include "../window/DXDevice.hpp"
#include "../util/DXError.hpp"
#include "DXTexture.hpp"
#include "../util/DebugUtil.hpp"

#include <d3d11.h>

Framebuffer::Framebuffer(ID3D11RenderTargetView* fbo, ID3D11RenderTargetView* depth, std::unique_ptr<Texture> texture)
{
}

static ID3D11Texture2D* create_texture(int width, int height, DXGI_FORMAT format, UINT bindFlags) {
	auto device = DXDevice::getDevice();

	D3D11_TEXTURE2D_DESC description;
	ZeroMemory(&description, sizeof(description));
	description.Width = width;
	description.Height = height;
	description.MipLevels = 1;
	description.ArraySize = 1;
	description.Format = format;
	description.SampleDesc.Count = 1;
	description.SampleDesc.Quality = 0;
	description.Usage = D3D11_USAGE_DEFAULT;
	description.BindFlags = bindFlags;
	description.CPUAccessFlags = 0;
	description.MiscFlags = 0;

	ID3D11Texture2D* tex = nullptr;

	CHECK_ERROR2(device->CreateTexture2D(&description, nullptr, &tex),
		L"Failed to create texture");

	return tex;
}

Framebuffer::Framebuffer(uint width, uint height, bool alpha) :
	m_width(0),
	m_height(0)
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

	auto device = DXDevice::getDevice();

	releaseResources();

	m_p_texture = std::make_unique<Texture>(create_texture(m_width, m_height, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET));
	CHECK_ERROR1(device->CreateRenderTargetView(m_p_texture->getId(), nullptr, &m_p_renderTarget));

	m_p_depthTexture = create_texture(m_width, m_height, DXGI_FORMAT_D32_FLOAT, D3D11_BIND_DEPTH_STENCIL);
	CHECK_ERROR1(device->CreateDepthStencilView(m_p_depthTexture, nullptr, &m_p_depthStencil));

	SET_DEBUG_OBJECT_NAME(m_p_renderTarget, "Frame buffer");
	SET_DEBUG_OBJECT_NAME(m_p_depthStencil, "Depth stencil");
}

void Framebuffer::bind() {
	DXDevice::setRenderTarget(m_p_renderTarget, m_p_depthStencil);
}

void Framebuffer::unbind() {
	DXDevice::resetRenderTarget();
}

void Framebuffer::releaseResources() {
	if (m_p_renderTarget) m_p_renderTarget->Release();
	if (m_p_depthStencil) m_p_depthStencil->Release();
	if (m_p_depthTexture) m_p_depthTexture->Release();
}

#endif // USE_DIRECTX