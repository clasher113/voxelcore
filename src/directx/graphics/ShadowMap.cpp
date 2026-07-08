#ifdef USE_DIRECTX

#include "ShadowMap.hpp"

#include "directx/window/Device.hpp"
#include "directx/util/Error.hpp"
#include "directx/util/DebugUtil.hpp"

ShadowMap::ShadowMap(int resolution) : resolution(resolution) {

    ID3D11Device* const device = Device::getDevice();

    D3D11_TEXTURE2D_DESC depthBufferDesc{
        /* UINT Width */					resolution,
        /* UINT Height */					resolution,
        /* UINT MipLevels */				1U,
        /* UINT ArraySize */				1U,
        /* DXGI_FORMAT Format */			DXGI_FORMAT_R32_TYPELESS,
        /* DXGI_SAMPLE_DESC SampleDesc */{
        /* UINT Count */	1U,
        /* UINT Quality */	0U
    },
        /* D3D11_USAGE Usage */				D3D11_USAGE_DEFAULT,
        /* UINT BindFlags */				D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
        /* UINT CPUAccessFlags */			0U,
        /* UINT MiscFlags */				0U
    };

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{
        DXGI_FORMAT_D32_FLOAT,
        D3D11_DSV_DIMENSION_TEXTURE2D,
        0, 0
    };

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{
        DXGI_FORMAT_R32_FLOAT,
        D3D11_SRV_DIMENSION_TEXTURE2D,
        0, 1
    };

    CHECK_ERROR2(device->CreateTexture2D(&depthBufferDesc, nullptr, &m_p_depthMap),
        L"Failed to create texture");
    CHECK_ERROR1(device->CreateDepthStencilView(m_p_depthMap, &dsvDesc, &m_p_depthStencil));
    CHECK_ERROR1(device->CreateShaderResourceView(m_p_depthMap, &srvDesc, &m_p_resourceView));
    SET_DEBUG_OBJECT_NAME(m_p_depthMap, "Shadow map");
    SET_DEBUG_OBJECT_NAME(m_p_depthStencil, "Shadow DSV");
    SET_DEBUG_OBJECT_NAME(m_p_resourceView, "Shadow RSV");
}

ShadowMap::~ShadowMap() {
    m_p_depthMap->Release();
    m_p_resourceView->Release();
    m_p_depthStencil->Release();
}

void ShadowMap::bind() {
    ID3D11RenderTargetView* nullRTV = nullptr;
    Device::setRenderTargets(&nullRTV, m_p_depthStencil);
    Device::clearDepth();
}

void ShadowMap::unbind() {
    Device::resetRenderTarget();
}

ID3D11ShaderResourceView* ShadowMap::getSRV() const {
    return m_p_resourceView;
}

int ShadowMap::getResolution() const {
    return resolution;
}

#endif // USE_DIRECTX