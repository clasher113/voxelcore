#ifdef USE_DIRECTX
#include "DXTexture.hpp"
#include "../window/DXDevice.hpp"
#include "../util/DXError.hpp"
#include "../../graphics/core/ImageData.hpp"
#include "../util/DebugUtil.hpp"
#include "../util/TextureUtil.hpp"

#include <stdexcept>

uint Texture::MAX_RESOLUTION = 8192;
constexpr UINT MAX_MIP_LEVEL = 3u;

static UINT GetNumMipLevels(UINT width, UINT height) {
    UINT numLevels = 1;
    while (width > 32 && height > 32) {
        if (++numLevels >= MAX_MIP_LEVEL) return MAX_MIP_LEVEL;
        width = std::max(width / 2, 1U);
        height = std::max(height / 2, 1U);
    }
    return numLevels;
}

Texture::Texture(ID3D11Texture2D* texture) :
    m_p_texture(texture),
    m_description()
{
    auto device = DXDevice::getDevice();
    CHECK_ERROR2(device->CreateShaderResourceView(m_p_texture, nullptr, &m_p_resourceView),
        L"Failed to create shader resource view");

    m_p_texture->GetDesc(&m_description);

    if (m_description.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS) {
        auto context = DXDevice::getContext();
        context->GenerateMips(m_p_resourceView);
    }
    SET_DEBUG_OBJECT_NAME(m_p_texture, "Texture 2D");
    SET_DEBUG_OBJECT_NAME(m_p_resourceView, "Resource View");
}

Texture::Texture(ubyte* data, uint width, uint height, ImageFormat format) {
    ZeroMemory(&m_description, sizeof(D3D11_TEXTURE2D_DESC));
    m_description.Width = width;
    m_description.Height = height;
    m_description.MipLevels = 0;
    m_description.ArraySize = 1;
    m_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_description.SampleDesc.Count = 1;
    m_description.SampleDesc.Quality = 0;
    m_description.CPUAccessFlags = 0;
    m_description.Usage = D3D11_USAGE_DEFAULT;
    m_description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    m_description.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    auto device = DXDevice::getDevice();
    CHECK_ERROR2(device->CreateTexture2D(&m_description, nullptr, &m_p_texture),
        L"Failed to create texture");

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
    srvDesc.Format = m_description.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = GetNumMipLevels(width, height);

    CHECK_ERROR2(device->CreateShaderResourceView(m_p_texture, &srvDesc, &m_p_resourceView),
        L"Failed to create shader resource view");

    auto context = DXDevice::getContext();

    reload(data);

    context->GenerateMips(m_p_resourceView);

    SET_DEBUG_OBJECT_NAME(m_p_texture, "Texture 2D");
    SET_DEBUG_OBJECT_NAME(m_p_resourceView, "Resource View");
}

Texture::~Texture() {
    m_p_texture->Release();
    m_p_resourceView->Release();
}

void Texture::setNearestFilter() {

}

void Texture::bind(unsigned int shaderType, UINT startSlot) const {
    auto context = DXDevice::getContext();
    if (shaderType & VERTEX)	context->VSSetShaderResources(startSlot, 1u, &m_p_resourceView);
    if (shaderType & PIXEL)		context->PSSetShaderResources(startSlot, 1u, &m_p_resourceView);
    if (shaderType & GEOMETRY)	context->GSSetShaderResources(startSlot, 1u, &m_p_resourceView);
}

void Texture::reload(ubyte* data) {
    auto context = DXDevice::getContext();
    context->UpdateSubresource(m_p_texture, 0u, nullptr, data, m_description.Width * (m_description.Format == DXGI_FORMAT_R8G8B8A8_UNORM ? 4 : 3), 0u);
}

std::unique_ptr<ImageData> Texture::readData() {
    auto data = std::make_unique<ubyte[]>(m_description.Width * m_description.Height * 4);
    ID3D11Texture2D* staged = nullptr;
    TextureUtil::stageTexture(m_p_texture, &staged);
    TextureUtil::readPixels(staged, data.get());
    staged->Release();
    return std::make_unique<ImageData>(
        ImageFormat::rgba8888, m_description.Width, m_description.Height, data.release()
    );
}

ID3D11Texture2D* Texture::getId() const {
    return m_p_texture;
}

ID3D11ShaderResourceView* Texture::getResourceView() const {
    return m_p_resourceView;
}

std::unique_ptr<Texture> Texture::from(const ImageData* image) {
    uint width = image->getWidth();
    uint height = image->getHeight();
    const void* data = image->getData();
    return std::make_unique<Texture>((ubyte*)data, width, height, image->getFormat());
}

#endif // USE_DIRECTX