#ifdef USE_DIRECTX
#include "Device.hpp"

#include "directx/graphics/LineRenderer.hpp"
#include "directx/util/Error.hpp"
#include "directx/util/AdapterReader.hpp"

#include <iostream>

#pragma comment (lib, "D3DCompiler.lib")
#pragma comment (lib, "DXGI.lib")
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxguid.lib")

void Device::initialize(HWND window, UINT windowWidth, UINT windowHeight) {
	if (s_m_initialized) {
		std::cout << "Device already initialized" << std::endl;
		return;
	}
	s_m_windowHandle = window;
	s_m_windowWidth = windowWidth;
	s_m_windowHeight = windowHeight;

	createDevice();
	createSwapChain();
	createResources();
	resizeViewPort(0.f, 0.f, windowWidth, windowHeight);
	LineRenderer::initialize(s_m_device.Get());
	s_m_initialized = true;
}

void Device::terminate() {
	if (!s_m_initialized) {
		std::cout << "Device not initialized" << std::endl;
		return;
	}
	LineRenderer::terminate();

	s_m_swapChain.Reset();
	s_m_renderTargetView.Reset();
	s_m_depthStencilView.Reset();
	s_m_depthStencilState.Reset();
	s_m_rasterizerState.Reset();
	s_m_blendState.Reset();
	s_m_samplerPointWrap.Reset();
	s_m_samplerLinearWrap.Reset();
	s_m_samplerLinearClamp.Reset();
	s_m_samplerPointClamp.Reset();
	s_m_samplerStateComparison.Reset();
	s_m_device.Reset();
	s_m_context->Flush();
	s_m_context.Reset();
#ifdef _DEBUG
	CHECK_ERROR1(s_m_debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL),
		"Cant get live objects", false);
	s_m_debug.Reset();
#endif // _DEBUG
	s_m_initialized = false;
}

void Device::createDevice() {
	UINT creationFlags = 0U;

#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	static const D3D_FEATURE_LEVEL featureLevels[] = {
		//D3D_FEATURE_LEVEL_12_1,
		//D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

	s_m_p_adapterData = AdapterReader::chooseAdapter();

	CHECK_ERROR2(D3D11CreateDevice(
		(s_m_p_adapterData ? s_m_p_adapterData->m_adapter.Get() : nullptr),
		(s_m_p_adapterData == nullptr ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_UNKNOWN),
		nullptr,
		creationFlags,
		featureLevels,
		static_cast<UINT>(std::size(featureLevels)),
		D3D11_SDK_VERSION,
		s_m_device.GetAddressOf(),
		&s_m_featureLevel,
		s_m_context.GetAddressOf()
	), L"Failed to create D3D11 device");

#ifndef NDEBUG
	if (SUCCEEDED(s_m_device.As(&s_m_debug))) {
		Microsoft::WRL::ComPtr<ID3D11InfoQueue> d3dInfoQueue;
		if (SUCCEEDED(s_m_debug.As(&d3dInfoQueue))) {
#ifdef _DEBUG
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
			D3D11_MESSAGE_ID hide[] = {
				D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
				D3D11_MESSAGE_ID_LIVE_OBJECT_SUMMARY
			};
			D3D11_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
			filter.DenyList.pIDList = hide;
			d3dInfoQueue->AddStorageFilterEntries(&filter);
		}
	}
#endif
}

void Device::createSwapChain() {
	Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice;
	CHECK_ERROR1(s_m_device->QueryInterface(IID_PPV_ARGS(dxgiDevice.GetAddressOf())));

	Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
	CHECK_ERROR1(dxgiDevice->GetParent(IID_PPV_ARGS(dxgiAdapter.GetAddressOf())));

	Microsoft::WRL::ComPtr<IDXGIFactory> dxgiFactory;
	CHECK_ERROR1(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

	DXGI_RATIONAL dxgiRational {
		/* UINT Numerator */	0U,
		/* UINT Denominator */	0U
	};

	DXGI_MODE_DESC dxgiModeDesc {
		/* UINT Width */								s_m_windowWidth,
		/* UINT Height */								s_m_windowHeight,
		/* DXGI_RATIONAL RefreshRate */					dxgiRational,
		/* DXGI_FORMAT Format */						DXGI_FORMAT_R8G8B8A8_UNORM,
		/* DXGI_MODE_SCANLINE_ORDER ScanlineOrdering */	DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
		/* DXGI_MODE_SCALING Scaling */					DXGI_MODE_SCALING_UNSPECIFIED
	};

	DXGI_SAMPLE_DESC dxgiSampleDesc {
		/* UINT Count */	1U,
		/* UINT Quality */	0U
	};

	DXGI_SWAP_CHAIN_DESC swapChainDesc {
		/* DXGI_MODE_DESC BufferDesc */		dxgiModeDesc,
		/* DXGI_SAMPLE_DESC SampleDesc */	dxgiSampleDesc,
		/* DXGI_USAGE BufferUsage */		DXGI_USAGE_RENDER_TARGET_OUTPUT,
		/* UINT BufferCount */				1U,
		/* HWND OutputWindow */				s_m_windowHandle,
		/* BOOL Windowed */					TRUE,
		/* DXGI_SWAP_EFFECT SwapEffect */	DXGI_SWAP_EFFECT_DISCARD,
		/* UINT Flags */					0U
	};

	CHECK_ERROR2(dxgiFactory->CreateSwapChain(s_m_device.Get(), &swapChainDesc, s_m_swapChain.GetAddressOf()),
		L"Failed to create swapchain");

	CHECK_ERROR1(dxgiFactory->MakeWindowAssociation(s_m_windowHandle, DXGI_MWA_NO_WINDOW_CHANGES));
}

void Device::createResources() {
	// create render target view
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer = getSurface();

	CHECK_ERROR2(s_m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, s_m_renderTargetView.ReleaseAndGetAddressOf()),
		L"Failed to create render target view");

	CD3D11_TEXTURE2D_DESC depthTexDesc(DXGI_FORMAT_D32_FLOAT, s_m_windowWidth, s_m_windowHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencil;
	CHECK_ERROR1(s_m_device->CreateTexture2D(&depthTexDesc, nullptr, depthStencil.GetAddressOf()));

	PRINT_ERROR2(s_m_device->CreateDepthStencilView(depthStencil.Get(), nullptr, s_m_depthStencilView.ReleaseAndGetAddressOf()),
		L"Failed to create depth stencil view");

	resetRenderTarget();

	// create depth stencil state
	ZeroMemory(&s_m_depthStencilStateDesc, sizeof(s_m_depthStencilStateDesc));

	s_m_depthStencilStateDesc.DepthEnable = true;
	s_m_depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
	s_m_depthStencilStateDesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;

	updateDepthStencilState();

	// create blend state
	ZeroMemory(&s_m_renderTargetBlendDesc, sizeof(s_m_renderTargetBlendDesc));

	s_m_renderTargetBlendDesc.BlendEnable = true;
	s_m_renderTargetBlendDesc.SrcBlend = D3D11_BLEND::D3D11_BLEND_SRC_ALPHA;
	s_m_renderTargetBlendDesc.DestBlend = D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA;
	s_m_renderTargetBlendDesc.BlendOp = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;

	s_m_renderTargetBlendDesc.SrcBlendAlpha = D3D11_BLEND::D3D11_BLEND_ONE;
	s_m_renderTargetBlendDesc.DestBlendAlpha = D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA;
	s_m_renderTargetBlendDesc.BlendOpAlpha = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
	s_m_renderTargetBlendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL;

	updateBlendState();

	// create rasterizer state
	ZeroMemory(&s_m_rasterizerStateDesc, sizeof(s_m_rasterizerStateDesc));

	s_m_rasterizerStateDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
	s_m_rasterizerStateDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE;
	s_m_rasterizerStateDesc.MultisampleEnable = true;
	updateRasterizerState();

	// create sampler state
	D3D11_SAMPLER_DESC samplerDesc {
		/* D3D11_FILTER Filter */					D3D11_FILTER_MIN_MAG_MIP_POINT,
		/* D3D11_TEXTURE_ADDRESS_MODE AddressU */	D3D11_TEXTURE_ADDRESS_WRAP,
		/* D3D11_TEXTURE_ADDRESS_MODE AddressV */	D3D11_TEXTURE_ADDRESS_WRAP,
		/* D3D11_TEXTURE_ADDRESS_MODE AddressW */	D3D11_TEXTURE_ADDRESS_WRAP,
		/* FLOAT MipLODBias */						0.f,
		/* UINT MaxAnisotropy */					0U,
		/* D3D11_COMPARISON_FUNC ComparisonFunc */	D3D11_COMPARISON_NEVER,
		/* FLOAT BorderColor[4] */					{ 0.f, 0.f, 0.f, 0.f },
		/* FLOAT MinLOD */							0.f,
		/* FLOAT MaxLOD */							D3D11_FLOAT32_MAX
	};

	PRINT_ERROR2(s_m_device->CreateSamplerState(&samplerDesc, s_m_samplerPointWrap.ReleaseAndGetAddressOf()),
		L"Failed to create sampler");

	s_m_context->VSSetSamplers(0, 1, s_m_samplerPointWrap.GetAddressOf());
	s_m_context->PSSetSamplers(0, 1, s_m_samplerPointWrap.GetAddressOf());

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

	PRINT_ERROR2(s_m_device->CreateSamplerState(&samplerDesc, s_m_samplerLinearWrap.ReleaseAndGetAddressOf()),
		L"Failed to create sampler");

	s_m_context->VSSetSamplers(1, 1, s_m_samplerLinearWrap.GetAddressOf());
	s_m_context->PSSetSamplers(1, 1, s_m_samplerLinearWrap.GetAddressOf());

	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

	PRINT_ERROR2(s_m_device->CreateSamplerState(&samplerDesc, s_m_samplerLinearClamp.ReleaseAndGetAddressOf()),
		L"Failed to create sampler");

	s_m_context->VSSetSamplers(3, 1, s_m_samplerLinearClamp.GetAddressOf());
	s_m_context->PSSetSamplers(3, 1, s_m_samplerLinearClamp.GetAddressOf());

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

	PRINT_ERROR2(s_m_device->CreateSamplerState(&samplerDesc, s_m_samplerPointClamp.ReleaseAndGetAddressOf()),
		L"Failed to create sampler");

	s_m_context->VSSetSamplers(4, 1, s_m_samplerPointClamp.GetAddressOf());
	s_m_context->PSSetSamplers(4, 1, s_m_samplerPointClamp.GetAddressOf());

	samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	samplerDesc.MaxAnisotropy = 1;
	for (size_t i = 0; i < 4; i++) {
		samplerDesc.BorderColor[i] = 1.f;
	}

	PRINT_ERROR2(s_m_device->CreateSamplerState(&samplerDesc, s_m_samplerStateComparison.ReleaseAndGetAddressOf()),
		L"Failed to create sampler");
	s_m_context->PSSetSamplers(2, 1, s_m_samplerStateComparison.GetAddressOf());
}

void Device::onDeviceLost() {
	terminate();
	initialize(s_m_windowHandle, s_m_windowWidth, s_m_windowHeight);
}

void Device::updateRasterizerState() {
	PRINT_ERROR2(s_m_device->CreateRasterizerState(&s_m_rasterizerStateDesc, s_m_rasterizerState.ReleaseAndGetAddressOf()),
		L"Failed to create rasterizer state");
	s_m_context->RSSetState(s_m_rasterizerState.Get());
}

void Device::updateDepthStencilState() {
	PRINT_ERROR2(s_m_device->CreateDepthStencilState(&s_m_depthStencilStateDesc, s_m_depthStencilState.ReleaseAndGetAddressOf()),
		L"Failed to create depth stencil state");
	s_m_context->OMSetDepthStencilState(s_m_depthStencilState.Get(), 0);
}

void Device::updateBlendState() {
	D3D11_BLEND_DESC blendStateDesc {
		/* BOOL AlphaToCoverageEnable */						false,
		/* BOOL IndependentBlendEnable */						false,
		/* D3D11_RENDER_TARGET_BLEND_DESC RenderTarget[8] */	0
	};
	blendStateDesc.RenderTarget[0] = s_m_renderTargetBlendDesc;

	PRINT_ERROR2(s_m_device->CreateBlendState(&blendStateDesc, s_m_blendState.ReleaseAndGetAddressOf()),
		L"Failed to create blend state");

	s_m_context->OMSetBlendState(s_m_blendState.Get(), NULL, 0xffffffff);
}

void Device::clearContext() {
	s_m_context->OMSetRenderTargets(0, nullptr, nullptr);
	s_m_renderTargetView.Reset();
	s_m_depthStencilView.Reset();
	s_m_rasterizerState.Reset();
	s_m_blendState.Reset();
	s_m_context->Flush();
}

void Device::onWindowResize(UINT windowWidth, UINT windowHeight) {
	clearContext();

	s_m_windowWidth = windowWidth;
	s_m_windowHeight = windowHeight;
	
	HRESULT hr = s_m_swapChain->ResizeBuffers(0, s_m_windowWidth, s_m_windowHeight, DXGI_FORMAT_UNKNOWN, 0);

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
		CHECK_ERROR3(s_m_device->GetDeviceRemovedReason(), L"Device removed", false);
		onDeviceLost();
		return;
	}

	PRINT_ERROR1(hr);

	createResources();

	resizeViewPort(0.f, 0.f, windowWidth, windowHeight);
}

void Device::resizeViewPort(FLOAT x, FLOAT y, FLOAT width, FLOAT height, FLOAT depthNear, FLOAT depthFar) {
	D3D11_VIEWPORT viewport{ x, y, width, height, depthNear, depthFar };
	s_m_context->RSSetViewports(1, &viewport);
}

void Device::setScissorRect(LONG x, LONG y, LONG width, LONG height) {
	D3D11_RECT rect{ x, y, x + width, y + height };
	s_m_context->RSSetScissorRects(1, &rect);
}

void Device::clear() {
	clearColor();
	clearDepth();
}

void Device::clearColor() {
	for (size_t i = 0; i < s_m_renderTargetViewsCount; i++) {
		s_m_context->ClearRenderTargetView(s_m_p_currentRenderTargetViews[i], s_m_clearColor);
	}
}

void Device::clearDepth() {
	if (!s_m_p_currentDepthStencilView) return;
	s_m_context->ClearDepthStencilView(s_m_p_currentDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
}

void Device::display() {
	HRESULT hr = s_m_swapChain->Present(s_m_swapInterval, 0);

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
		CHECK_ERROR3(s_m_device->GetDeviceRemovedReason(), L"Device removed", false);
		onDeviceLost();
	}
	else {
		PRINT_ERROR1(hr);
	}
}

void Device::setDepthTest(bool flag) {
	s_m_context->OMSetRenderTargets(s_m_renderTargetViewsCount, s_m_p_currentRenderTargetViews,
		(flag ? s_m_p_currentDepthStencilView : nullptr));
}

void Device::setCullFace(bool flag) {
	s_m_rasterizerStateDesc.CullMode = (flag ? D3D11_CULL_MODE::D3D11_CULL_FRONT : D3D11_CULL_MODE::D3D11_CULL_NONE);
	updateRasterizerState();
}

void Device::setWriteDepthEnabled(bool flag) {
	s_m_depthStencilStateDesc.DepthWriteMask = (flag ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO);
	updateDepthStencilState();
}

void Device::setBlendFunc(D3D11_BLEND srcBlend, D3D11_BLEND dstBlend, D3D11_BLEND_OP blendOp, D3D11_BLEND srcBlendAlpha, 
	D3D11_BLEND dstBlendAlpha, D3D11_BLEND_OP blendOpAlpha) 
{
	s_m_renderTargetBlendDesc.SrcBlend = srcBlend;
	s_m_renderTargetBlendDesc.DestBlend = dstBlend;
	s_m_renderTargetBlendDesc.BlendOp = blendOp;

	s_m_renderTargetBlendDesc.SrcBlendAlpha = srcBlendAlpha;
	s_m_renderTargetBlendDesc.DestBlendAlpha = dstBlendAlpha;
	s_m_renderTargetBlendDesc.BlendOpAlpha = blendOpAlpha;
	updateBlendState();
}

void Device::setScissorTest(BOOL flag) {
	s_m_rasterizerStateDesc.ScissorEnable = flag;
	updateRasterizerState();
}

void Device::setSwapInterval(UINT interval) {
	if (interval > 4) {
		std::cout << "SyncInterval must be less than or equal to 4" << std::endl;
		return;
	}
	s_m_swapInterval = interval;
}

void Device::setDepthBuffer(ID3D11DepthStencilView* depthStencil) {
	s_m_p_currentDepthStencilView = depthStencil;
	s_m_context->OMSetRenderTargets(s_m_renderTargetViewsCount, s_m_p_currentRenderTargetViews, s_m_p_currentDepthStencilView);
}

void Device::setRenderTargets(ID3D11RenderTargetView* const* renderTargets, ID3D11DepthStencilView* depthStencil, size_t renderTargetsCount) {
	s_m_renderTargetViewsCount = renderTargetsCount;
	for (size_t i = 0; i < s_m_renderTargetViewsCount; i++) {
		s_m_p_currentRenderTargetViews[i] = renderTargets[i];
	}
	s_m_p_currentDepthStencilView = depthStencil;
	s_m_context->OMSetRenderTargets(s_m_renderTargetViewsCount, s_m_p_currentRenderTargetViews, s_m_p_currentDepthStencilView);
}

void Device::setClearColor(float r, float g, float b, float a) {
	s_m_clearColor = { { { r, g, b, a } } };
}

void Device::resetRenderTarget() {
	setRenderTargets(s_m_renderTargetView.GetAddressOf(), s_m_depthStencilView.Get());
}

Microsoft::WRL::ComPtr<ID3D11Texture2D> Device::getSurface() {
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	CHECK_ERROR1(s_m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));
	return backBuffer;
}

const AdapterData& Device::getAdapterData() {
	return *s_m_p_adapterData;
}

#endif // USE_DIRECTX