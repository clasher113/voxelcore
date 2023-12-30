#ifdef USE_DIRECTX
#include "DXSkybox.hpp"
#include "DXMesh.hpp"
#include "../window/DXDevice.hpp"
#include "../util/DXError.hpp"
#include "../ConstantBuffers.hpp"
#include "DXShader.hpp"
#include "..\math\DXMathHelper.hpp"
#include "..\..\window\Window.h"

#include <glm\glm.hpp>
#include <d3d11_1.h>

#ifndef M_PI
#define M_PI 3.141592
#endif // M_PI

Skybox::Skybox(uint size, Shader* shader) :
    m_size(size),
    m_p_shader(shader)
{
    auto device = DXDevice::getDevice();

    D3D11_TEXTURE2D_DESC description;
    ZeroMemory(&description, sizeof(description));
    description.Width = m_size;
    description.Height = m_size;
    description.MipLevels = 1;
    description.ArraySize = 6;
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.SampleDesc.Quality = 0;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    description.CPUAccessFlags = 0;
    description.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    CHECK_ERROR2(device->CreateTexture2D(&description, nullptr, &m_p_texture),
        L"Failed to create texture");

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = description.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    
    CHECK_ERROR2(device->CreateShaderResourceView(m_p_texture, &srvDesc, &m_p_resourceView),
        L"Failed to create shader resource view");

    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
    renderTargetViewDesc.Format = description.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
    renderTargetViewDesc.Texture2DArray.MipSlice = 0;
    renderTargetViewDesc.Texture2DArray.ArraySize = 1;

    for (size_t i = 0; i < 6; i++) {
        renderTargetViewDesc.Texture2DArray.FirstArraySlice = i;
        CHECK_ERROR1(device->CreateRenderTargetView(m_p_texture, &renderTargetViewDesc, &m_p_renderTargets[i]));
    }

    float vertices[]{
        -1.0f, -1.0f, 1.0f, 1.0f,-1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,-1.0f, 1.0f, 1.0f
    };
    vattr attrs[]{ 2, 0 };
    m_p_mesh = new Mesh(vertices, 6, attrs);
}

Skybox::~Skybox() {
    m_p_texture->Release();
    for (size_t i = 0; i < 6; i++) {
        m_p_renderTargets[i]->Release();
    }
    m_p_resourceView->Release();
    delete m_p_mesh;
}

void Skybox::draw(Shader* shader) {
    auto context = DXDevice::getContext();
    context->PSSetShaderResources(0, 1, &m_p_resourceView);
    m_p_mesh->draw();
    unbind();
}

void Skybox::refresh(float t, float mie, uint quality) {
    auto context = DXDevice::getContext();
    
    ready = true;
    DXDevice::resizeViewPort(0, 0, m_size, m_size);
    m_p_shader->use();
    cbSkybox->bind(ShaderType::PIXEL);
    const DirectX::XMFLOAT3 xaxs[] = {
        {0.0f, 0.0f, -1.0f},
        {0.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 0.0f},

        {-1.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
    };
    const DirectX::XMFLOAT3 yaxs[] = {
        {0.0f,-1.0f, 0.0f},
        {0.0f,-1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},

        {0.0f, 0.0f,-1.0f},
        {0.0f,-1.0f, 0.0f},
        {0.0f,-1.0f, 0.0f},
    };

    const DirectX::XMFLOAT3 zaxs[] = {
        {1.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},

        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, -1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    t *= M_PI * 2.0f;

    cbSkybox->data.quality = quality;
    cbSkybox->data.mie = mie;
    cbSkybox->data.fog = mie - 1.0f;
    cbSkybox->data.lightDir = glm2dxm(glm::normalize(glm::vec3(sin(t), -cos(t), -0.7f)));
    for (uint face = 0; face < 6; face++) {
        context->OMSetRenderTargets(1, &m_p_renderTargets[face], nullptr);
        cbSkybox->data.xAxis = xaxs[face];
        cbSkybox->data.yAxis = yaxs[face];
        cbSkybox->data.zAxis = zaxs[face];
        cbSkybox->applyChanges();
        m_p_mesh->draw();
    }

    DXDevice::rebindRenderTarget();
    DXDevice::resizeViewPort(0, 0, Window::width, Window::height);
}

void Skybox::bind() const {
    auto context = DXDevice::getContext();
    context->VSSetShaderResources(1, 1, &m_p_resourceView);
    context->PSSetShaderResources(1, 1, &m_p_resourceView);
}

void Skybox::unbind() const {
    auto context = DXDevice::getContext();
    ID3D11ShaderResourceView* nullSRV = { nullptr };
    context->PSSetShaderResources(0, 1, &nullSRV);
}

#endif // USE_DIRECTX