#ifdef USE_DIRECTX
#include "TextureUtil.hpp"
#include "../window/DXDevice.hpp"

HRESULT TextureUtil::stageTexture(ID3D11Texture2D* src, ID3D11Texture2D** dst) {
	auto device = DXDevice::getDevice();
	auto context = DXDevice::getContext();

	D3D11_TEXTURE2D_DESC desc{};
	src->GetDesc(&desc);

	if (desc.ArraySize > 1 || desc.MipLevels > 1) return S_FALSE;

	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.BindFlags = 0;

	HRESULT hr = device->CreateTexture2D(&desc, nullptr, dst);
	if (FAILED(hr)) return hr;

	context->CopyResource(*dst, src);

	return S_OK;
}

HRESULT TextureUtil::readPixels(ID3D11Texture2D* src, void* dst) {
	auto context = DXDevice::getContext();

	D3D11_MAPPED_SUBRESOURCE resourceDesc{};
	D3D11_TEXTURE2D_DESC textureDesc{};
	src->GetDesc(&textureDesc);
	if (textureDesc.Usage != D3D11_USAGE_STAGING || textureDesc.CPUAccessFlags != D3D11_CPU_ACCESS_READ) return S_FALSE;

	HRESULT hr = context->Map(src, 0, D3D11_MAP_READ, 0, &resourceDesc);
	if (FAILED(hr)) return hr;

	unsigned int rowPitch = textureDesc.Width * 4;
	unsigned int location = 0;

	if (rowPitch == resourceDesc.RowPitch) {
		memcpy(dst, resourceDesc.pData, textureDesc.Width * textureDesc.Height * 4);
	}
	else {
		const unsigned char* source = static_cast<const unsigned char*>(resourceDesc.pData);
		unsigned char* dest = static_cast<unsigned char*>(dst);
		for (int i = 0; i < textureDesc.Height; ++i) {
			memcpy(&dest[i * (textureDesc.Width * 4)], &source[location], textureDesc.Width * 4);
			location += resourceDesc.RowPitch;
		}
	}
	context->Unmap(src, 0);

	return S_OK;
}

#endif // USE_DIRECTX