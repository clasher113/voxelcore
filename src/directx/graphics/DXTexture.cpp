#ifdef USE_DIRECTX
#include "DXTexture.hpp"
#include "../window/DXDevice.hpp"
#include "../util/DXError.hpp"
#include "../../graphics/ImageData.h"

#include <stdexcept>

Texture::Texture(ID3D11Texture2D* texture, const D3D11_TEXTURE2D_DESC& description) :
    m_p_texture(texture),
    m_description(description)
{
    auto device = DXDevice::getDevice();
    DXError::checkError(device->CreateShaderResourceView(m_p_texture, nullptr, &m_p_resourceView),
        L"Failed to create shader resource view");
}

Texture::Texture(ubyte* data, UINT width, UINT height, DXGI_FORMAT format) {
    m_description.Width = width;
    m_description.Height = height;
    m_description.MipLevels = 1;
    m_description.ArraySize = 1;
    m_description.Format = format;
    m_description.SampleDesc.Count = 1;
    m_description.Usage = D3D11_USAGE_DEFAULT;
    m_description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    m_description.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    D3D11_SUBRESOURCE_DATA sub{};
    sub.pSysMem = data,
    sub.SysMemPitch = width * (format == DXGI_FORMAT_R8G8B8A8_UNORM ? 4 : 3);

    auto device = DXDevice::getDevice();
    DXError::checkError(device->CreateTexture2D(&m_description, &sub, &m_p_texture),
        L"Failed to create texture");
    DXError::checkError(device->CreateShaderResourceView(m_p_texture, nullptr, &m_p_resourceView),
        L"Failed to create shader resource view");

    auto context = DXDevice::getContext();
    context->GenerateMips(m_p_resourceView);
}

Texture::~Texture() {
    m_p_texture->Release();
    m_p_resourceView->Release();
}

void Texture::bind() const {
    auto context = DXDevice::getContext();
    context->PSSetShaderResources(0, 1, &m_p_resourceView);
}

void Texture::reload(ubyte* data) {
    // TODO
}

Texture* Texture::from(const ImageData* image) {
    uint width = image->getWidth();
    uint height = image->getHeight();
    DXGI_FORMAT format;
    const void* data = image->getData();
    switch (image->getFormat()) {
        case ImageFormat::rgb888: format = DXGI_FORMAT_D24_UNORM_S8_UINT; break;
        case ImageFormat::rgba8888: format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
    default:
        throw std::runtime_error("unsupported image data format");
    }
    return new Texture((ubyte*)data, width, height, format);
}

#endif // USE_DIRECTX