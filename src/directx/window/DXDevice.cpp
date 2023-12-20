#ifdef USE_DIRECTX
#include "DXDevice.hpp"
#include "../util/DXError.hpp"
#include "../util/AdapterReader.hpp"

#include <DirectXColors.h>
#include <iostream>

UINT DXDevice::s_m_swapInterval = 1;
HWND DXDevice::s_m_windowHandle = 0;
unsigned int DXDevice::s_m_windowWidth = 0, DXDevice::s_m_windowHeight = 0;

D3D_FEATURE_LEVEL DXDevice::s_m_featureLevel = D3D_FEATURE_LEVEL_9_1;
DXGI_ADAPTER_DESC DXDevice::s_m_adapterDesc;
Microsoft::WRL::ComPtr<ID3D11Device1> DXDevice::s_m_device = nullptr;
Microsoft::WRL::ComPtr<ID3D11DeviceContext1>  DXDevice::s_m_context = nullptr;

Microsoft::WRL::ComPtr<IDXGISwapChain1>  DXDevice::s_m_swapChain = nullptr;
Microsoft::WRL::ComPtr<ID3D11RenderTargetView> DXDevice::s_m_renderTargetView = nullptr;
Microsoft::WRL::ComPtr<ID3D11DepthStencilView> DXDevice::s_m_depthStencilView = nullptr;
Microsoft::WRL::ComPtr<ID3D11DepthStencilState> DXDevice::s_m_depthStencilState = nullptr;
Microsoft::WRL::ComPtr<ID3D11RasterizerState> DXDevice::s_m_rasterizerState = nullptr;
Microsoft::WRL::ComPtr<ID3D11BlendState1> DXDevice::s_m_blendState = nullptr;

bool DXDevice::initialize(HWND window, UINT windowWidth, UINT windowHeight) {
    s_m_windowHandle = window;
    s_m_windowWidth = windowWidth;
    s_m_windowHeight = windowHeight;

    createDevice();
    createSwapChain();
    createResources();
    resizeViewPort(0.f, 0.f, windowWidth, windowHeight);

    return true;
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
    AdapterData* performanceAdapter = &adapters.front();
    size_t adapterIndex = 0;

    do {
        if (performanceAdapter == nullptr) performanceAdapter = &adapters[adapterIndex];
        else if (performanceAdapter->description.DedicatedVideoMemory < adapters[adapterIndex].description.DedicatedVideoMemory) {
            performanceAdapter = &adapters[adapterIndex];
        }
        adapterIndex++;
    } while (adapterIndex < adapters.size());
    
    D3D_DRIVER_TYPE driverType = (performanceAdapter->pAdapter == nullptr ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_UNKNOWN);

    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

    DXError::checkError(D3D11CreateDevice (
        performanceAdapter->pAdapter,
        driverType,
        nullptr,
        creationFlags,
        featureLevels,
        static_cast<UINT>(std::size(featureLevels)),
        D3D11_SDK_VERSION,
        device.ReleaseAndGetAddressOf(),
        &s_m_featureLevel,
        context.ReleaseAndGetAddressOf()
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
            };
            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    DXError::checkError(device.As(&s_m_device));
    DXError::checkError(context.As(&s_m_context));

}

void DXDevice::createSwapChain() {
    const UINT backBufferWidth = static_cast<UINT>(s_m_windowWidth);
    const UINT backBufferHeight = static_cast<UINT>(s_m_windowHeight);
    const DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    constexpr UINT backBufferCount = 2;

    Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice;
    DXError::checkError(s_m_device.As(&dxgiDevice));

    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
    DXError::checkError(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

    DXGI_ADAPTER_DESC adapterDesc{};
    dxgiAdapter->GetDesc(&adapterDesc);
    
    std::wcout << L"Renderer: " << adapterDesc.Description << std::endl; 

    Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
    DXError::checkError(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = backBufferWidth;
    swapChainDesc.Height = backBufferHeight;
    swapChainDesc.Format = backBufferFormat;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = backBufferCount;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
    fsSwapChainDesc.Windowed = TRUE;

    DXError::checkError(dxgiFactory->CreateSwapChainForHwnd(
        s_m_device.Get(),
        s_m_windowHandle,
        &swapChainDesc,
        &fsSwapChainDesc,
        nullptr,
        s_m_swapChain.ReleaseAndGetAddressOf()
    ));

    DXError::checkError(dxgiFactory->MakeWindowAssociation(s_m_windowHandle, DXGI_MWA_NO_ALT_ENTER));
}

void DXDevice::createResources() {
    const UINT backBufferWidth = static_cast<UINT>(s_m_windowWidth);
    const UINT backBufferHeight = static_cast<UINT>(s_m_windowHeight);
    const DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // create render target view
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    DXError::checkError(s_m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));
    DXError::checkError(s_m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, s_m_renderTargetView.ReleaseAndGetAddressOf()));

    CD3D11_TEXTURE2D_DESC depthTexDesc(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencil;
    DXError::checkError(s_m_device->CreateTexture2D(&depthTexDesc, nullptr, depthStencil.GetAddressOf()));

    DXError::checkError(s_m_device->CreateDepthStencilView(depthStencil.Get(), nullptr, s_m_depthStencilView.ReleaseAndGetAddressOf()),
        L"Failed to create depth stencil view");

    // create depth stencil state
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
    ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;

    DXError::checkError(s_m_device->CreateDepthStencilState(&depthStencilDesc, s_m_depthStencilState.GetAddressOf()),
        L"Failed to create depth stencil state");

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
    rtbd.DestBlendAlpha = D3D11_BLEND::D3D11_BLEND_ZERO;
    rtbd.BlendOpAlpha = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
    rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL;

    blendStateDesc.RenderTarget[0] = rtbd;

    DXError::checkError(s_m_device->CreateBlendState1(&blendStateDesc, s_m_blendState.GetAddressOf()),
        L"Failed to create blend state");

    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    UINT sampleMask = 0xffffffff;
    s_m_context->OMSetBlendState(s_m_blendState.Get(), blendFactor, sampleMask);

    // create rasterizer state
    D3D11_RASTERIZER_DESC rasterizerDesc{};
    ZeroMemory(&rasterizerDesc, sizeof(rasterizerDesc));

    rasterizerDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_FRONT;

    DXError::checkError(s_m_device->CreateRasterizerState(&rasterizerDesc, s_m_rasterizerState.GetAddressOf()));

    s_m_context->RSSetState(s_m_rasterizerState.Get());

    // create sampler state
    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    ID3D11SamplerState* samplerState;
    s_m_device->CreateSamplerState(&samplerDesc, &samplerState);

    s_m_context->PSSetSamplers(0, 1, &samplerState);
}

void DXDevice::onDeviceLost() {
    s_m_depthStencilView.Reset();
    s_m_renderTargetView.Reset();
    s_m_swapChain.Reset();
    s_m_context.Reset();
    s_m_device.Reset();

    initialize(s_m_windowHandle, s_m_windowWidth, s_m_windowHeight);
}

void DXDevice::clearContext() {
    s_m_context->OMSetRenderTargets(0, nullptr, nullptr);
    s_m_renderTargetView.Reset();
    s_m_depthStencilView.Reset();
    s_m_context->Flush();
}

void DXDevice::onFullScreenToggle(bool isFullScreen, UINT windowWidth, UINT windowHeight) {
    if (isFullScreen) {
        SetWindowLongPtr(s_m_windowHandle, GWL_STYLE, WS_OVERLAPPEDWINDOW);
        SetWindowLongPtr(s_m_windowHandle, GWL_EXSTYLE, 0);

        ShowWindow(s_m_windowHandle, SW_SHOWNORMAL);

        SetWindowPos(s_m_windowHandle, HWND_TOP, 0, 0, windowWidth, windowHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
    else {
        SetWindowLongPtr(s_m_windowHandle, GWL_STYLE, WS_POPUP);
        SetWindowLongPtr(s_m_windowHandle, GWL_EXSTYLE, WS_EX_TOPMOST);

        SetWindowPos(s_m_windowHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        ShowWindow(s_m_windowHandle, SW_SHOWMAXIMIZED);
    }
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
        DXError::checkError(hr);
    }

    createResources();

    resizeViewPort(0.f, 0.f, windowWidth, windowHeight);
}

void DXDevice::resizeViewPort(FLOAT x, FLOAT y, FLOAT width, FLOAT height, FLOAT depthNear, FLOAT depthFar) {
    D3D11_VIEWPORT viewport = { x, y, width, height, depthNear, depthFar };
    s_m_context->RSSetViewports(1, &viewport);
}

void DXDevice::setScissorRect(LONG x, LONG y, LONG width, LONG height) {
    D3D11_RECT rect;
    rect.left = x;
    rect.top = y;
    rect.right = x + width;
    rect.bottom = y + height;
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
        DXError::checkError(hr);
    }
}

void DXDevice::setDepthTest(bool enabled) {
    s_m_context->OMSetRenderTargets(1, s_m_renderTargetView.GetAddressOf(), (enabled ? s_m_depthStencilView.Get() : nullptr));
}

void DXDevice::setCullFace(BOOL enabled) {
    D3D11_RASTERIZER_DESC rasterizerDesc{};
    s_m_rasterizerState->GetDesc(&rasterizerDesc);
    rasterizerDesc.CullMode = (enabled ? D3D11_CULL_MODE::D3D11_CULL_FRONT : D3D11_CULL_MODE::D3D11_CULL_BACK);
    s_m_rasterizerState->Release();
    DXError::checkError(s_m_device->CreateRasterizerState(&rasterizerDesc, s_m_rasterizerState.GetAddressOf()));
    s_m_context->RSSetState(s_m_rasterizerState.Get());
}

void DXDevice::setScissorTest(BOOL enabled) {
    //D3D11_RASTERIZER_DESC rasterizerDesc{};
    //s_m_rasterizerState->GetDesc(&rasterizerDesc);
    //rasterizerDesc.ScissorEnable = enabled;
    //s_m_rasterizerState->Release();
    //DXError::checkError(s_m_device->CreateRasterizerState(&rasterizerDesc, s_m_rasterizerState.GetAddressOf()));
    //s_m_context->RSSetState(s_m_rasterizerState.Get());
}

void DXDevice::setSwapInterval(UINT interval) {
    s_m_swapInterval = interval;
}

#endif // USE_DIRECTX