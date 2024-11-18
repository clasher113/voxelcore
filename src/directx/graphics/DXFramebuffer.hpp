#ifndef DX_FRAMEBUFFER_HPP
#define DX_FRAMEBUFFER_HPP

#include "typedefs.hpp"

#include <memory>

class Texture;
struct ID3D11Texture2D;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

class Framebuffer {
public:
	Framebuffer(ID3D11RenderTargetView* fbo, ID3D11RenderTargetView* depth, std::unique_ptr<Texture> texture);
	Framebuffer(uint width, uint height, bool alpha = false);
	~Framebuffer();

	Texture* getTexture() const;

	void resize(uint width, uint height);
	void bind();
	void unbind();

private:
	uint m_width, m_height;

	std::unique_ptr<Texture> m_p_texture;
	ID3D11Texture2D* m_p_depthTexture;
	ID3D11RenderTargetView* m_p_renderTarget;
	ID3D11DepthStencilView* m_p_depthStencil;

	void releaseResources();
};

#endif // !DX_FRAMEBUFFER_HPP