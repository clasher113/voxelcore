#ifdef USE_DIRECTX
#include "DXDevice.hpp"
#include "../graphics/DXLine.hpp"
#include "../util/DXError.hpp"
#include "../util/AdapterReader.hpp"
#include "../../util/stringutil.h"

#include <DirectXColors.h>
#include <iostream>

void DXDevice::initialize(HWND window, UINT windowWidth, UINT windowHeight) {
	s_m_windowHandle = window;
	s_m_windowWidth = windowWidth;
	s_m_windowHeight = windowHeight;

	createDevice();
	createSwapChain();
	createResources();
	resizeViewPort(0.f, 0.f, windowWidth, windowHeight);
	DXLine::initialize();
}

void DXDevice::terminate() {
	DXLine::terminate();
#ifdef _DEBUG
	ID3D11Debug* debugDev;
	s_m_device->QueryInterface(IID_PPV_ARGS(&debugDev));
	CHECK_ERROR1(debugDev->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL), "Not all objects was been destroyed", false);
	debugDev->Release();
#endif // _DEBUG
	s_m_device.Reset();
	s_m_context.Reset();
	s_m_swapChain.Reset();
	s_m_renderTargetView.Reset();
	s_m_depthStencilView.Reset();
	s_m_depthStencilState.Reset();
	s_m_rasterizerState.Reset();
	s_m_blendState.Reset();
	s_m_samplerState.Reset();
	s_m_samplerStateLinear.Reset();
}

void DXDevice::createDevice() {
	UINT creationFlags = 0;

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

	auto adapters = AdapterReader::GetAdapters();
	AdapterData* performanceAdapter = nullptr;
	size_t adapterIndex = 0;

	do {
		if (performanceAdapter == nullptr) performanceAdapter = &adapters[adapterIndex];
		else if (performanceAdapter->m_description.DedicatedVideoMemory < adapters[adapterIndex].m_description.DedicatedVideoMemory) {
			performanceAdapter = &adapters[adapterIndex];
		}
		adapterIndex++;
	} while (adapterIndex < adapters.size());

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

	CHECK_ERROR2(D3D11CreateDevice(
		(performanceAdapter == nullptr ? nullptr : performanceAdapter->m_adapter.Get()),
		(performanceAdapter == nullptr ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_UNKNOWN),
		nullptr,
		creationFlags,
		featureLevels,
		static_cast<UINT>(std::size(featureLevels)),
		D3D11_SDK_VERSION,
		device.GetAddressOf(),
		&s_m_featureLevel,
		context.GetAddressOf()
	), L"Failed to create D3D11 device");

#ifndef NDEBUG
	Microsoft::WRL::ComPtr<ID3D11Debug> d3dDebug;
	if (SUCCEEDED(device.As(&d3dDebug))) {
		Microsoft::WRL::ComPtr<ID3D11InfoQueue> d3dInfoQueue;
		if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue))) {
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

	CHECK_ERROR1(device.As(&s_m_device));
	CHECK_ERROR1(context.As(&s_m_context));
}

void DXDevice::createSwapChain() {
	Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice;
	CHECK_ERROR1(s_m_device.As(&dxgiDevice));

	Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
	CHECK_ERROR1(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

	Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
	CHECK_ERROR1(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = s_m_windowWidth;
	swapChainDesc.Height = s_m_windowHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc{};
	fsSwapChainDesc.Windowed = TRUE;

	CHECK_ERROR1(dxgiFactory->CreateSwapChainForHwnd(
		s_m_device.Get(),
		s_m_windowHandle,
		&swapChainDesc,
		&fsSwapChainDesc,
		nullptr,
		s_m_swapChain.GetAddressOf()
	));

	CHECK_ERROR1(dxgiFactory->MakeWindowAssociation(s_m_windowHandle, DXGI_MWA_NO_ALT_ENTER));
}

void DXDevice::createResources() {
	// create render target view
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer = getSurface();
	CHECK_ERROR2(s_m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, s_m_renderTargetView.ReleaseAndGetAddressOf()),
		L"Failed to create render target view");

	CD3D11_TEXTURE2D_DESC depthTexDesc(DXGI_FORMAT_D24_UNORM_S8_UINT, s_m_windowWidth, s_m_windowHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencil;
	CHECK_ERROR1(s_m_device->CreateTexture2D(&depthTexDesc, nullptr, depthStencil.GetAddressOf()));

	CHECK_ERROR2(s_m_device->CreateDepthStencilView(depthStencil.Get(), nullptr, s_m_depthStencilView.ReleaseAndGetAddressOf()),
		L"Failed to create depth stencil view", false);

	// create depth stencil state
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;

	CHECK_ERROR3(s_m_device->CreateDepthStencilState(&depthStencilDesc, s_m_depthStencilState.ReleaseAndGetAddressOf()),
		L"Failed to create depth stencil state", false);

	s_m_context->OMSetRenderTargets(1, s_m_renderTargetView.GetAddressOf(), s_m_depthStencilView.Get());
	s_m_context->OMSetDepthStencilState(s_m_depthStencilState.Get(), 0);

	// create blend state
	D3D11_BLEND_DESC1 blendStateDesc{};
	ZeroMemory(&blendStateDesc, sizeof(blendStateDesc));

	D3D11_RENDER_TARGET_BLEND_DESC1 rtbd{};
	ZeroMemory(&rtbd, sizeof(rtbd));

	rtbd.BlendEnable = true;
	rtbd.SrcBlend = D3D11_BLEND::D3D11_BLEND_SRC_ALPHA;
	rtbd.DestBlend = D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA;
	rtbd.BlendOp = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;

	rtbd.SrcBlendAlpha = D3D11_BLEND::D3D11_BLEND_ONE;
	rtbd.DestBlendAlpha = D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA;
	rtbd.BlendOpAlpha = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
	rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL;

	blendStateDesc.RenderTarget[0] = rtbd;

	CHECK_ERROR3(s_m_device->CreateBlendState1(&blendStateDesc, s_m_blendState.ReleaseAndGetAddressOf()),
		L"Failed to create blend state", false);

	float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
	UINT sampleMask = 0xffffffff;
	s_m_context->OMSetBlendState(s_m_blendState.Get(), blendFactor, sampleMask);

	// create rasterizer state
	ZeroMemory(&s_m_rasterizerStateDesc, sizeof(s_m_rasterizerStateDesc));

	s_m_rasterizerStateDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
	s_m_rasterizerStateDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_FRONT;

	CHECK_ERROR3(s_m_device->CreateRasterizerState(&s_m_rasterizerStateDesc, s_m_rasterizerState.ReleaseAndGetAddressOf()),
		L"Failed to create rasterizer state", false);

	s_m_context->RSSetState(s_m_rasterizerState.Get());

	// create sampler state
	D3D11_SAMPLER_DESC samplerDesc{};
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MipLODBias = 0.f;
	samplerDesc.MinLOD = 0.f;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	CHECK_ERROR3(s_m_device->CreateSamplerState(&samplerDesc, s_m_samplerState.ReleaseAndGetAddressOf()),
		L"Failed to create sampler", false);

	s_m_context->VSSetSamplers(0, 1, s_m_samplerState.GetAddressOf());
	s_m_context->PSSetSamplers(0, 1, s_m_samplerState.GetAddressOf());

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

	CHECK_ERROR3(s_m_device->CreateSamplerState(&samplerDesc, s_m_samplerStateLinear.ReleaseAndGetAddressOf()),
		L"Failed to create sampler", false);

	s_m_context->VSSetSamplers(1, 1, s_m_samplerStateLinear.GetAddressOf());
	s_m_context->PSSetSamplers(1, 1, s_m_samplerStateLinear.GetAddressOf());
}

void DXDevice::onDeviceLost() {
	terminate();
	initialize(s_m_windowHandle, s_m_windowWidth, s_m_windowHeight);
}

void DXDevice::updateRasterizerState() {
	CHECK_ERROR1(s_m_device->CreateRasterizerState(&s_m_rasterizerStateDesc, s_m_rasterizerState.ReleaseAndGetAddressOf()));
	s_m_context->RSSetState(s_m_rasterizerState.Get());
}

void DXDevice::clearContext() {
	s_m_context->OMSetRenderTargets(0, nullptr, nullptr);
	s_m_renderTargetView.Reset();
	s_m_depthStencilView.Reset();
	s_m_rasterizerState.Reset();
	s_m_blendState.Reset();
	s_m_context->Flush();
}

void DXDevice::onWindowResize(UINT windowWidth, UINT windowHeight) {
	clearContext();

	s_m_windowWidth = windowWidth;
	s_m_windowHeight = windowHeight;

	HRESULT hr = s_m_swapChain->ResizeBuffers(0, s_m_windowWidth, s_m_windowHeight, DXGI_FORMAT_UNKNOWN, 0);

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
		onDeviceLost();
		return;
	}
	else {
		CHECK_ERROR1(hr);
	}

	createResources();

	resizeViewPort(0.f, 0.f, windowWidth, windowHeight);
}

void DXDevice::resizeViewPort(FLOAT x, FLOAT y, FLOAT width, FLOAT height, FLOAT depthNear, FLOAT depthFar) {
	D3D11_VIEWPORT viewport{ x, y, width, height, depthNear, depthFar };
	s_m_context->RSSetViewports(1, &viewport);
}

void DXDevice::setScissorRect(LONG x, LONG y, LONG width, LONG height) {
	D3D11_RECT rect{ x, y, x + width, y + height };
	s_m_context->RSSetScissorRects(1, &rect);
}

void DXDevice::clear() {
	s_m_context->ClearRenderTargetView(s_m_renderTargetView.Get(), DirectX::Colors::CornflowerBlue);
	s_m_context->ClearDepthStencilView(s_m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
}

void DXDevice::clearDepth() {
	s_m_context->ClearDepthStencilView(s_m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
}

void DXDevice::display() {
	HRESULT hr = s_m_swapChain->Present(s_m_swapInterval, 0);

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
		onDeviceLost();
	}
	else {
		CHECK_ERROR1(hr);
	}
}

void DXDevice::setDepthTest(bool enabled) {
	s_m_context->OMSetRenderTargets(1, s_m_renderTargetView.GetAddressOf(), (enabled ? s_m_depthStencilView.Get() : nullptr));
}

void DXDevice::setCullFace(bool enabled) {
	s_m_rasterizerStateDesc.CullMode = (enabled ? D3D11_CULL_MODE::D3D11_CULL_FRONT : D3D11_CULL_MODE::D3D11_CULL_NONE);
	updateRasterizerState();
}

void DXDevice::setScissorTest(BOOL enabled) {
	s_m_rasterizerStateDesc.ScissorEnable = enabled;
	updateRasterizerState();
}

void DXDevice::setSwapInterval(UINT interval) {
	if (interval > 4) {
		std::cout << "SyncInterval must be less than or equal to 4" << std::endl;
		return;
	}
	s_m_swapInterval = interval;
}

void DXDevice::rebindRenderTarget() {
	s_m_context->OMSetRenderTargets(1, s_m_renderTargetView.GetAddressOf(), s_m_depthStencilView.Get());
}

Microsoft::WRL::ComPtr<ID3D11Texture2D> DXDevice::getSurface() {
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	CHECK_ERROR1(s_m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));
	return backBuffer;
}

DXGI_ADAPTER_DESC DXDevice::getAdapterDesc() {
	Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice;
	CHECK_ERROR1(s_m_device.As(&dxgiDevice));

	Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
	CHECK_ERROR1(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

	DXGI_ADAPTER_DESC adapterDesc{};
	dxgiAdapter->GetDesc(&adapterDesc);
	return adapterDesc;
}

#endif // USE_DIRECTX