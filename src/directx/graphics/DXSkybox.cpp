#ifdef USE_DIRECTX
#include "DXSkybox.hpp"
#include "DXMesh.hpp"
#include "../window/DXDevice.hpp"
#include "../util/DXError.hpp"
#include "../ConstantBuffer.hpp"
#include "DXShader.hpp"
#include "../math/DXMathHelper.hpp"
#include "../../window/Window.h"

#include <glm\glm.hpp>
#include <d3d11_1.h>

#ifndef M_PI
#define M_PI 3.141592
#endif // M_PI

const DirectX::XMFLOAT3 xaxs[] = {
    { 0.f, 0.f,-1.f },
    { 0.f, 0.f, 1.f },
    {-1.f, 0.f, 0.f },

    {-1.f, 0.f, 0.f },
    {-1.f, 0.f, 0.f },
    { 1.f, 0.f, 0.f },
};
const DirectX::XMFLOAT3 yaxs[] = {
    { 0.f,-1.f, 0.f },
    { 0.f,-1.f, 0.f },
    { 0.f, 0.f, 1.f },

    { 0.f, 0.f,-1.f },
    { 0.f,-1.f, 0.f },
    { 0.f,-1.f, 0.f },
};
const DirectX::XMFLOAT3 zaxs[] = {
    { 1.f, 0.f, 0.f },
    {-1.f, 0.f, 0.f },
    { 0.f,-1.f, 0.f },

    { 0.f, 1.f, 0.f },
    { 0.f, 0.f,-1.f },
    { 0.f, 0.f, 1.f },
};

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

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = description.Format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
    rtvDesc.Texture2DArray.MipSlice = 0;
    rtvDesc.Texture2DArray.ArraySize = 1;

    for (size_t i = 0; i < 6; i++) {
        rtvDesc.Texture2DArray.FirstArraySlice = i;
        CHECK_ERROR1(device->CreateRenderTargetView(m_p_texture, &rtvDesc, &m_p_renderTargets[i]));
    }

    float vertices[] {
        -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f,  1.0f, 1.0f, 1.0f, -1.0f
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
    shader->applyChanges();
    DXDevice::setDepthTest(false);
    DXDevice::getContext()->PSSetShaderResources(0, 1, &m_p_resourceView);
    m_p_mesh->draw();
    unbind();
    DXDevice::setDepthTest(true);
}

void Skybox::refresh(float t, float mie, uint quality) {   
    ready = true;
    DXDevice::resizeViewPort(0, 0, m_size, m_size);
    m_p_shader->use();

    t *= M_PI * 2.0f;

    m_p_shader->uniform1i("c_quality", quality);
    m_p_shader->uniform1f("c_mie", mie);
    m_p_shader->uniform1f("c_fog", mie - 1.0f);
    m_p_shader->uniform3f("c_lightDir", glm::normalize(glm::vec3(sin(t), -cos(t), -0.7f)));
    for (uint face = 0; face < 6; face++) {
        DXDevice::getContext()->OMSetRenderTargets(1, &m_p_renderTargets[face], nullptr);
        m_p_shader->uniform3f("c_xAxis", xaxs[face]);
        m_p_shader->uniform3f("c_yAxis", yaxs[face]);
        m_p_shader->uniform3f("c_zAxis", zaxs[face]);

        m_p_shader->applyChanges();
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
    context->VSSetShaderResources(1, 1, &nullSRV);
    context->PSSetShaderResources(1, 1, &nullSRV);
}

#endif // USE_DIRECTX