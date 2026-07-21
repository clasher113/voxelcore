#pragma once

#include "typedefs.hpp"
#include "graphics/core/commons.hpp"

#include <memory>

class Texture;
struct ID3D11Texture2D;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

class Framebuffer : public Bindable {
public:
	Framebuffer(std::unique_ptr<Texture> texture);
	Framebuffer(uint width, uint height, bool alpha = false);
	~Framebuffer();

	Texture* getTexture() const;
	ID3D11Texture2D* getDepthTexture();

	void resize(uint width, uint height);
	void bind(size_t index);
	virtual void bind() override;
	virtual void unbind() override;

	std::shared_ptr<Texture> getSharedTexture() const;
	uint getWidth() const;
	uint getHeight() const;

	ID3D11RenderTargetView* getRTV(size_t index = 0);
	ID3D11DepthStencilView* getDSV();
private:
	uint m_width, m_height;
	size_t m_renderTargetCount;

	std::shared_ptr<Texture> m_p_texture;
	ID3D11Texture2D* m_p_depthTexture;
	ID3D11RenderTargetView** m_p_renderTarget;
	ID3D11DepthStencilView* m_p_depthStencil;

	void releaseResources();
};