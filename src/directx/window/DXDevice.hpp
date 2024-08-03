#ifndef DX_DEVICE_HPP
#define DX_DEVICE_HPP

#define NOMINMAX
#include <d3d11_1.h>
#include <wrl/client.h>
#include <DirectXColors.h>
#undef DELETE

class AdapterData;

class DXDevice {
public:
	static void initialize(HWND window, UINT windowWidth, UINT windowHeight);
	static void terminate();

	static void onWindowResize(UINT windowWidth, UINT windowHeight);
	static void resizeViewPort(FLOAT x, FLOAT y, FLOAT width, FLOAT height, FLOAT depthNear = 0.f, FLOAT depthFar = 1.f);
	static void setScissorRect(LONG x, LONG y, LONG width, LONG height);

	static void clear();
	static void clearDepth();
	static void display();

	static void setDepthTest(bool flag);
	static void setCullFace(bool flag);
	static void setWriteDepthEnabled(bool flag);
	static void setBlendFunc(D3D11_BLEND srcBlend, D3D11_BLEND dstBlend, D3D11_BLEND_OP blendOp, 
		D3D11_BLEND srcBlendAlpha, D3D11_BLEND dstBlendAlpha, D3D11_BLEND_OP blendOpAlpha);
	static void setScissorTest(BOOL flag);
	static void setSwapInterval(UINT interval);
	static void setRenderTarget(ID3D11RenderTargetView* renderTarget, ID3D11DepthStencilView* depthStencil = nullptr);
	static void setClearColor(float r, float g, float b, float a);

	static void resetRenderTarget();

	static const HWND getWindowHandle() { return s_m_windowHandle; }
	static ID3D11Device* getDevice() { return s_m_device.Get(); };
	static ID3D11DeviceContext* getContext() { return s_m_context.Get(); };
	static Microsoft::WRL::ComPtr<ID3D11Texture2D> getSurface();
	static const AdapterData& getAdapterData();
private:
	static inline bool s_m_initialized = false;
	static inline UINT s_m_swapInterval = 1;
	static inline HWND s_m_windowHandle = 0;
	static inline UINT s_m_windowWidth = 0, s_m_windowHeight = 0;
	static inline DirectX::XMVECTORF32 s_m_clearColor = DirectX::Colors::CornflowerBlue;

	static inline D3D_FEATURE_LEVEL									s_m_featureLevel = D3D_FEATURE_LEVEL_9_1;
	static inline const AdapterData*								s_m_p_adapterData = nullptr;
	static inline D3D11_RASTERIZER_DESC								s_m_rasterizerStateDesc;
	static inline D3D11_DEPTH_STENCIL_DESC							s_m_depthStencilStateDesc;
	static inline D3D11_RENDER_TARGET_BLEND_DESC					s_m_renderTargetBlendDesc;
	static inline Microsoft::WRL::ComPtr<ID3D11Device>				s_m_device = nullptr;
	static inline Microsoft::WRL::ComPtr<ID3D11DeviceContext>		s_m_context = nullptr;
	static inline Microsoft::WRL::ComPtr<IDXGISwapChain>			s_m_swapChain = nullptr;
#ifdef _DEBUG
	static inline Microsoft::WRL::ComPtr<ID3D11Debug>				s_m_debug = nullptr;
#endif // _DEBUG
	static inline Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	s_m_renderTargetView = nullptr;
	static inline Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	s_m_depthStencilView = nullptr;

	static inline Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	s_m_depthStencilState = nullptr;
	static inline Microsoft::WRL::ComPtr<ID3D11RasterizerState>		s_m_rasterizerState = nullptr;
	static inline Microsoft::WRL::ComPtr<ID3D11BlendState>			s_m_blendState = nullptr;
	static inline Microsoft::WRL::ComPtr<ID3D11SamplerState>		s_m_samplerState = nullptr;
	static inline Microsoft::WRL::ComPtr<ID3D11SamplerState>		s_m_samplerStateLinear = nullptr;

	static inline ID3D11RenderTargetView* s_m_p_currentRenderTargetView = nullptr;
	static inline ID3D11DepthStencilView* s_m_p_currentDepthStencilView = nullptr;

	static void createDevice();
	static void createResources();
	static void createSwapChain();
	static void clearContext();
	static void onDeviceLost();

	static void updateRasterizerState();
	static void updateDepthStencilState();
	static void updateBlendState();
};

#endif // !DX_DEVICE_HPP