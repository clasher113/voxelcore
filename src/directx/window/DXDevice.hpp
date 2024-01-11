#ifndef DX_DEVICE_HPP
#define DX_DEVICE_HPP

#define NOMINMAX
#include <d3d11_1.h>
#include <wrl/client.h>

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
	static void setDepthTest(bool enabled);
	static void setCullFace(bool enabled);
	static void setScissorTest(BOOL enabled);
	static void setSwapInterval(UINT interval);
	static void rebindRenderTarget();

	static const HWND getWindowHandle() { return s_m_windowHandle; }
	static ID3D11Device1* getDevice() { return s_m_device.Get(); };
	static ID3D11DeviceContext1* getContext() { return s_m_context.Get(); };
	static Microsoft::WRL::ComPtr<ID3D11Texture2D> getSurface();
	static DXGI_ADAPTER_DESC getAdapterDesc();
private:
	static UINT s_m_swapInterval;
	static HWND s_m_windowHandle;
	static UINT s_m_windowWidth, s_m_windowHeight;

	static D3D_FEATURE_LEVEL                               s_m_featureLevel;
	static DXGI_ADAPTER_DESC							   s_m_adapterDesc;
	static D3D11_RASTERIZER_DESC						   s_m_rasterizerStateDesc;
	static Microsoft::WRL::ComPtr<ID3D11Device1>           s_m_device;
	static Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    s_m_context;
	static Microsoft::WRL::ComPtr<IDXGISwapChain1>         s_m_swapChain;
	static Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  s_m_renderTargetView;
	static Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  s_m_depthStencilView;
	static Microsoft::WRL::ComPtr<ID3D11DepthStencilState> s_m_depthStencilState;
	static Microsoft::WRL::ComPtr<ID3D11RasterizerState>   s_m_rasterizerState;
	static Microsoft::WRL::ComPtr<ID3D11BlendState1>	   s_m_blendState;
	static Microsoft::WRL::ComPtr<ID3D11SamplerState>	   s_m_samplerState;
	static Microsoft::WRL::ComPtr<ID3D11SamplerState>	   s_m_samplerStateLinear;

	static void createDevice();
	static void createResources();
	static void createSwapChain();
	static void clearContext();
	static void onDeviceLost();
	static void updateRasterizerState();
};

#endif // !DX_DEVICE_HPP