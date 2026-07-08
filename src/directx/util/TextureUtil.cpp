#ifdef USE_DIRECTX
#include "TextureUtil.hpp"

#include "directx/window/Device.hpp"

bool TextureUtil::stageTexture(ID3D11Texture2D* src, ID3D11Texture2D** dst) {
	ID3D11Device* const device = Device::getDevice();
	ID3D11DeviceContext* const context = Device::getContext();

	D3D11_TEXTURE2D_DESC desc{};
	src->GetDesc(&desc);

	//if (desc.ArraySize > 1 || desc.MipLevels > 1) return S_FALSE;

	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.BindFlags = 0U;
	desc.MiscFlags = 0U;

	if (device->CreateTexture2D(&desc, nullptr, dst) != S_OK) 
		return false;

	context->CopyResource(*dst, src);

	return true;
}

bool TextureUtil::readPixels(ID3D11Texture2D* src, void* dst, bool flipY) {
	ID3D11DeviceContext* const context = Device::getContext();

	D3D11_MAPPED_SUBRESOURCE resourceDesc{};
	D3D11_TEXTURE2D_DESC textureDesc{};
	src->GetDesc(&textureDesc);
	if (textureDesc.Usage != D3D11_USAGE_STAGING || textureDesc.CPUAccessFlags != D3D11_CPU_ACCESS_READ) return S_FALSE;

	if (context->Map(src, 0, D3D11_MAP_READ, 0, &resourceDesc) != S_OK)
		return false;

	unsigned int rowPitch = textureDesc.Width * 4;

	if (rowPitch == resourceDesc.RowPitch && !flipY) {
		memcpy(dst, resourceDesc.pData, textureDesc.Width * textureDesc.Height * 4);
	}
	else {
		const unsigned char* source = static_cast<const unsigned char*>(resourceDesc.pData);
		unsigned char* dest = static_cast<unsigned char*>(dst);
		unsigned int location = (flipY ? (textureDesc.Height - 1) * resourceDesc.RowPitch : 0);
		for (int i = 0; i < textureDesc.Height; ++i) {
			memcpy(&dest[i * (textureDesc.Width * 4)], &source[location], textureDesc.Width * 4);
			location += resourceDesc.RowPitch * (flipY ? -1 : 1);
		}
	}
	context->Unmap(src, 0);

	return true;
}

#endif // USE_DIRECTX